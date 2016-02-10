#include "graph.h"

/* Initialize graph. */
int initialize(graph *g, char *filename, char format)
{
    // Set number of threads
    omp_set_num_threads(NTHREADS);

    // Check graph file name length
    if (strlen(filename) > FILENAMELEN)
    {
        fprintf(stderr, "Max file name length exceeded.\n");
        return 1;
    }

    // Open graph file
    strcpy(g->filename, filename);
    g->stream = fopen(g->filename, "r");

    // Check file stream
    if (g->stream == NULL)
    {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }

    // Get numbers of nodes and edges
    fread(&g->n, sizeof(unsigned long long), 1, g->stream);
    fread(&g->m, sizeof(unsigned long long), 1, g->stream);
    
    // Check number of edges
    if (g->m > 4294967296)
    {
        fprintf(stderr, "Too many edges to handle: %llu\n", g->m);
        return 1;
    }

    // Get graph format
    g->format = format;
    
    // If graph is in BADJBLK format
    if (g->format == BADJBLK)
    {
        // Get number of blocks
        fread(&g->nblks, sizeof(unsigned long long), 1, g->stream);
        
        // Get indices and first nodes
        g->indices = malloc(g->nblks * sizeof(unsigned long long));
        fread(g->indices, sizeof(unsigned long long), g->nblks, g->stream);
        g->firstnodes = malloc(g->nblks * sizeof(unsigned int));
        fread(g->firstnodes, sizeof(unsigned int), g->nblks, g->stream);
        
        // Initialize blocks
        unsigned int i;
        for (i = 0; i < NTHREADS; i++)
        {
            // Open file for block
            g->currblock[i] = fopen(g->filename, "r");
            g->currblockno[i] = i - NTHREADS + 1;

            // Check file stream        
            if (g->currblock[i] == NULL)
            {
                fprintf(stderr, "Could not open file.\n");
                return 1;
            }
        }
    }

    return 0;
}

/* Destroy graph. */
int destroy(graph *g)
{
    // Close graph file
    fclose(g->stream);

    // If graph is in BADJBLK format
    if (g->format == BADJBLK)
    {
        // Destroy blocks
        unsigned int i;
        for (i = 0; i < NTHREADS; i++)
        {
            // Close file for block
            fclose(g->currblock[i]);
        }
    }

    return 0;
}

/* Partition a BADJ graph into a BADJBLK graph. */
int partition(graph *g)
{ 
    // Test graph format
    if (g->format != BADJ)
    {
        fprintf(stderr, "Original graph must be in BADJ format.\n");
        return 1;
    }

    // Declare variables
    unsigned long long nblks = 0;
    unsigned int currnode = 0;
    unsigned long long indices[MAXBLKS];
    unsigned int firstnodes[MAXBLKS];
   
    // Initialize block
    unsigned int *block = malloc(BLOCKLEN);
    
    // For each block
    while (1)
    {
        // Set block index and first node
        indices[nblks] = ftello(g->stream);
        firstnodes[nblks] = currnode;

        // Read maximal block
        unsigned int intsread = fread(block, sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);
        unsigned int blocklen = 0;
        
        // Check for end of file
        if (intsread == 0)
        {
            break;
        }
        
        // Increment the number of blocks
        nblks++;
        if (nblks > MAXBLKS)
        {
            fprintf(stderr, "Too many blocks to handle.\n");
            return 1;
        }

        // For each node
        while (1)
        {
            // Get degree
            unsigned int deg = block[blocklen];

            // Test whether node fits in block
            if (blocklen + 1 + deg <= intsread)
            {
                // If so, increment block length
                blocklen = blocklen + 1 + deg;
                currnode++;
            }
            else
            {
                // Otherwise, unread node
                int extra = intsread - blocklen;
                fseek(g->stream, -extra*sizeof(unsigned int), SEEK_CUR);
                break;
            }
        }
    }

    // Create BADJBLK file
    char filename[FILENAMELEN];
    strcpy(filename, g->filename);
    strcat(filename, "blk");
    FILE *out = (FILE *) fopen(filename, "w");

    // Check file stream
    if (out == NULL)
    {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }

    // Write numbers of nodes, edges, and blocks
    fwrite(&g->n, sizeof(unsigned long long), 1, out);
    fwrite(&g->m, sizeof(unsigned long long), 1, out);
    fwrite(&nblks, sizeof(unsigned long long), 1, out);

    // Correct block indices
    unsigned int i;
    for (i = 0; i < nblks; i++)
    {
        indices[i] = indices[i] + (1 + nblks)*sizeof(unsigned long long) + (nblks + i)*sizeof(unsigned long);
    }

    // Write block indices and first nodes
    fwrite(indices, sizeof(unsigned long long), nblks, out);
    fwrite(firstnodes, sizeof(unsigned int), nblks, out);

    // Rewind BADJ file
    fseek(g->stream, sizeof(unsigned long long) * 2, SEEK_SET);
    currnode = 0;

    // For each block
    while (1)
    {
        // Read maximal block
        unsigned int intsread = fread(block, sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);
        unsigned int blocklen = 0;
        
        // Check for end of file
        if (intsread == 0)
        {
            break;
        }

        // For each node
        while (1)
        {
            // Get degree
            unsigned int deg = block[blocklen];

            // Test whether node fits in block
            if (blocklen + 1 + deg <= intsread)
            {
                // If so, increment block length
                blocklen = blocklen + 1 + deg;
                currnode++;
            }
            else
            {
                // Otherwise, unread node
                int extra = intsread - blocklen;
                fseek(g->stream, -extra*sizeof(unsigned int), SEEK_CUR);
                break;
            }
        }
        
        // Write block with delimiter
        fwrite(block, sizeof(unsigned int), blocklen, out);
        unsigned int delimiter = (unsigned int) -1;
        fwrite(&delimiter, sizeof(unsigned int), 1, out);
    }

    // Close BADJBLK file
    fclose(out);

    // Free block
    free(block);

    return 0;
}

/* Get the next block of the graph. */
int nextblock(graph *g, unsigned int threadno)
{
    // Test graph format
    if (g->format != BADJBLK)
    {
        fprintf(stderr, "Graph must be in BADJBLK format.\n");
        return 1;
    }

    // Increment block number
    g->currblockno[threadno] += NTHREADS;
    if (g->currblockno[threadno] > g->nblks)
    {
        g->currblockno[threadno] = threadno + 1;
    }

    // Set block file pointer
    fseeko(g->currblock[threadno], g->indices[g->currblockno[threadno]-1], SEEK_SET);

    // Set current node
    g->currnode[threadno] = g->firstnodes[g->currblockno[threadno]-1];

    return 0;
}

/* Get the next node of the block. */
unsigned int nextnode(graph *g, node *v, unsigned int threadno)
{
    // Test graph format
    if (g->format != BADJBLK)
    {
        fprintf(stderr, "Graph must be in BADJBLK format.\n");
        return -1;
    }

    // Read next degree
    fread(&v->deg, sizeof(unsigned int), 1, g->currblock[threadno]);

    // If there is another node
    if (v->deg != (unsigned int) -1)
    {
        // Read next node
        v->adj = malloc(v->deg * sizeof(unsigned int));
        fread(v->adj, sizeof(unsigned int), v->deg, g->currblock[threadno]);
        g->currnode[threadno]++;
        return (g->currnode[threadno] - 1);
    }
    
    // Otherwise, no next node
    return -1;
}

#include "graph.h"

/* Initialize graph. */
int initialize(graph *g, char *filename, char format)
{
    // Check file name length
    if (strlen(filename) > FILENAMELEN)
    {
        fprintf(stderr, "Max file name length exceeded.\n");
        return 1;
    }

    // Get graph format
    g->format = format;
    
    // Test graph format
    if (g->format == BADJ)
    {
        // Open BADJ file
        strcpy(g->filename, filename);    
        g->stream = (FILE *) fopen(g->filename, "r");
    
        // Check file stream
        if (g->stream == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return 1;
        }

        // Get numbers of nodes and edges
        fread(&g->n, sizeof(unsigned long long), 1, g->stream);
        fread(&g->m, sizeof(unsigned long long), 1, g->stream);
    }
    else if (g->format == BADJBLK)
    {
        // Open BADJBLK file
        strcpy(g->filename, filename);
        g->stream = (FILE *) fopen(g->filename, "r");

        // Check file stream
        if (g->stream == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return 1;
        }

        // Get numbers of nodes, edges, and blocks
        fread(&g->n, sizeof(unsigned long long), 1, g->stream);
        fread(&g->m, sizeof(unsigned long long), 1, g->stream);
        fread(&g->nblks, sizeof(unsigned long long), 1, g->stream);

        // Get indices and first nodes
        g->indices = malloc(g->nblks * sizeof(unsigned long long));
        fread(g->indices, sizeof(unsigned long long), g->nblks, g->stream);
        g->firstnodes = malloc(g->nblks * sizeof(unsigned int));
        fread(g->firstnodes, sizeof(unsigned int), g->nblks, g->stream);
    }

    // Check number of edges
    if (g->m > 4294967296)
    {
        fprintf(stderr, "Too many edges to handle: %llu\n", g->m);
        return 1;
    }

    // Initialize blocks
    unsigned int i;
    for (i = 0; i < NTHREADS; i++)
    {
        // Current blocks
        g->currblock[i] = (FILE *) fopen(g->filename, "r");
        g->currblockno[i] = i - NTHREADS + 1;

        // Check file stream        
        if (g->currblock[i] == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return 1;
        }
    }

    // Set number of threads
    omp_set_num_threads(NTHREADS);

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
    unsigned int nblocks = 0;
    unsigned int currnode = 0;
    unsigned long long indices[MAXBLKS];
    unsigned int firstnodes[MAXBLKS];
   
    // Initialize block
    unsigned int *block = malloc(BLOCKLEN);
    
    // Initialize degrees
    unsigned int *degrees = malloc(sizeof(unsigned int) * g->n);

    // Initialize indegrees
    unsigned int *indegrees = malloc(sizeof(unsigned int) * g->n);
    unsigned int i;
    for (i = 0; i < g->n; i++)
    {
        indegrees[i] = 0;
    }
   
    // First pass over the blocks
    while (1)
    {
        // Set index of block and its first node
        indices[nblocks] = ftello(g->stream);
        firstnodes[nblocks] = currnode;

        // Read maximal block
        unsigned int intsread = fread(block, sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);
        unsigned int blocklen = 0;
        
        // Check for end of file
        if (intsread == 0)
        {
            break;
        }
        
        // Increment the number of blocks
        nblocks++;
        if (nblocks > MAXBLKS)
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
                // If so, update degrees
                degrees[currnode] = deg;
                unsigned int j;
                for (j = 0; j < deg; j++)
                {
                    indegrees[block[blocklen + 1 + j]]++;
                }
                
                // and increment block length
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

    // Find top-indegree nodes
    unsigned int topnodes[TOPLEN];
    unsigned int j, k;
    for (j = 0; j < TOPLEN; j++)
    {
        topnodes[j] = (unsigned int) -1;
    }
    for (i = 0; i < g->n; i++)
    {
        for (j = 0; j < TOPLEN; j++)
        {
            if (topnodes[j] == (unsigned int) -1)
            {
                topnodes[j] = i;
                break;
            }
            else if (indegrees[i] > indegrees[topnodes[j]])
            {
                for (k = TOPLEN - 1; k > j; k--)
                {
                    topnodes[k] = topnodes[k-1];
                }
                topnodes[j] = i;
                break;
            }
        }
    }

    // Sort top-indegree nodes
    for (i = TOPLEN - 1; i > 0; i--)
    {
        for (j = 0; j < i; j++)
        {
            if (topnodes[j] > topnodes[j+1])
            {
                unsigned int tmp = topnodes[j];
                topnodes[j] = topnodes[j+1];
                topnodes[j+1] = tmp;
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
    unsigned long long nblocksl = (unsigned long long) nblocks;
    fwrite(&g->n, sizeof(unsigned long long), 1, out);
    fwrite(&g->m, sizeof(unsigned long long), 1, out);
    fwrite(&nblocksl, sizeof(unsigned long long), 1, out);

    // Update block indices to account for nblocks, indices, firstnodes, and delimiters
    for (i = 0; i < nblocks; i++)
    {
        indices[i] = indices[i] + (1 + nblocks)*sizeof(unsigned long long) + (nblocks + i)*sizeof(unsigned int);
    }

    // Write block indices and first nodes
    // TODO: Currently ignoring topnodes
    fwrite(indices, sizeof(unsigned long long), nblocks, out);
    fwrite(firstnodes, sizeof(unsigned int), nblocks, out);

    // Rewind BADJ file
    fseek(g->stream, sizeof(unsigned long long) * 2, SEEK_SET);
    currnode = 0;

    // Second pass over the blocks
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
        
        // Write block
        fwrite(block, sizeof(unsigned int), blocklen, out);
        unsigned int delimiter = (unsigned int) -1;
        fwrite(&delimiter, sizeof(unsigned int), 1, out);
    }

    // Close BADJBLK file
    fclose(out);

    // Free stuff
    free(block);
    free(degrees);
    free(indegrees);

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

/* Get the next node in the graph. */
unsigned int nextnode(graph *g, node *v, unsigned int threadno)
{
    // Test graph format
    if (g->format != BADJBLK)
    {
        fprintf(stderr, "Graph must be in BADJBLK format.\n");
        return -1;
    }

    // Read next degree
    unsigned int intsread = fread(&v->deg, sizeof(unsigned int), 1, g->currblock[threadno]);

    if (intsread > 0 && v->deg != (unsigned int) -1)
    {
        // Read next node
        v->adj = malloc(v->deg * sizeof(unsigned int));
        fread(v->adj, sizeof(unsigned int), v->deg, g->currblock[threadno]);
        g->currnode[threadno]++;
        return (g->currnode[threadno] - 1);
    }
    
    // No next node
    return -1;
}

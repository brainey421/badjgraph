#include "graph.h"

/* Initialize graph. */
int initialize(graph *g, char *filename, char badji)
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
        fprintf(stderr, "Could not open BADJ file.\n");
        return 1;
    }

    // Get numbers of nodes and edges
    fread(&g->n, sizeof(unsigned long long), 1, g->stream);
    fread(&g->m, sizeof(unsigned long long), 1, g->stream);
    
    // Check number of nodes
    if (g->n > MAXNODES)
    {
        fprintf(stderr, "Too many nodes to handle: %llu\n", g->n);
        return 1;
    }

    // Check for badji file
    g->badji = badji;
    
    // If graph has badji file
    if (g->badji)
    {
        // Open badji file
        char badjiname[FILENAMELEN];
        strcpy(badjiname, g->filename);
        strcat(badjiname, "i");
        FILE *badjistream = (FILE *) fopen(badjiname, "r");

        // Check file stream
        if (badjistream == NULL)
        {
            fprintf(stderr, "Could not open badji file.\n");
            return 1;
        }

        // Get number of blocks, block indices, and first nodes
        fread(&g->nblks, sizeof(unsigned long long), 1, badjistream);
        g->indices = malloc(g->nblks * sizeof(unsigned long long));
        fread(g->indices, sizeof(unsigned long long), g->nblks, badjistream);
        g->firstnodes = malloc(g->nblks * sizeof(unsigned int));
        fread(g->firstnodes, sizeof(unsigned int), g->nblks, badjistream);
        
        // Close badji file
        fclose(badjistream);

        if (g->nblks < NTHREADS)
        {
            fprintf(stderr, "Too many threads for too small a graph.\n");
            return 1;
        }

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
                fprintf(stderr, "Could not open BADJ file.\n");
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

    // If graph has badji file
    if (g->badji)
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

/* Transpose a BADJ graph. */
int transpose(graph *g, char *filename)
{
    // Allocate space for degrees in transpose
    unsigned int *degt = malloc(sizeof(unsigned int) * g->n);
    unsigned int *currdegt = malloc(sizeof(unsigned int) * g->n);
    unsigned int i;
    for (i = 0; i < g->n; i++)
    {
        degt[i] = 0;
        currdegt[i] = 0;
    }

    // For each node
    unsigned int deg;
    unsigned int *adj;
    for (i = 0; i < g->n; i++)
    {
        // Read adjacency list
        fread(&deg, sizeof(unsigned int), 1, g->stream);
        adj = malloc(sizeof(unsigned int) * deg);
        fread(adj, sizeof(unsigned int), deg, g->stream);

        // Increment degrees in transpose
        unsigned int j;
        for (j = 0; j < deg; j++)
        {
            degt[adj[j]]++;
        }
        free(adj);
    }

    // Allocate space for adjacency list in transpose
    unsigned int **adjt = malloc(sizeof(unsigned int *) * g->n);
    for (i = 0; i < g->n; i++)
    {
        adjt[i] = malloc(sizeof(unsigned int) * degt[i]);
    }

    // Rewind graph
    fseek(g->stream, 2*sizeof(unsigned long long), SEEK_SET);

    // For each node
    for (i = 0; i < g->n; i++)
    {
        // Read adjacency list
        fread(&deg, sizeof(unsigned int), 1, g->stream);
        adj = malloc(sizeof(unsigned int) * deg);
        fread(adj, sizeof(unsigned int), deg, g->stream);

        // Update adjacency list in transpose
        unsigned int j;
        for (j = 0; j < deg; j++)
        {
            adjt[adj[j]][currdegt[adj[j]]] = i;
            currdegt[adj[j]]++;
        }
        free(adj);
    }

    // Create BADJ file
    FILE *out = fopen(filename, "w");
    fwrite(&g->n, sizeof(unsigned long long), 1, out);
    fwrite(&g->m, sizeof(unsigned long long), 1, out);

    // For each node
    for (i = 0; i < g->n; i++)
    {
        // Write degree and adjacent nodes in transpose
        fwrite(&degt[i], sizeof(unsigned int), 1, out);
        fwrite(adjt[i], sizeof(unsigned int), degt[i], out);
    }

    // Clean up
    fclose(out);
    free(degt);
    free(currdegt);
    free(adjt);

    return 0;
}

/* Compute the locality of a BADJ graph. */
int locality(graph *g, unsigned int window, double *loc)
{
    // Initialize number of local references
    unsigned int refs = 0;

    // Initialize previous node
    unsigned int prev = ((unsigned int) - 1) / 2;

    // For each node
    unsigned int deg;
    unsigned int *adj;
    unsigned int i;
    for (i = 0; i < g->n; i++)
    {
        // Read adjacency list
        fread(&deg, sizeof(unsigned int), 1, g->stream);
        adj = malloc(sizeof(unsigned int) * deg);
        fread(adj, sizeof(unsigned int), deg, g->stream);

        // For each neighbor
        unsigned int j;
        for (j = 0; j < deg; j++)
        {
            // Increment number of local reference if node is within window
            if (abs(adj[j] - prev) < window)
            {
                refs++;
            }
            
            // Update previous node
            prev = adj[j];
        }
        free(adj);
    }

    // Find percentage of local references
    *loc = (double) refs / (double) (g->m - 1);

    return 0;
}

/* Create a badji file for a BADJ graph. */
int badjindex(graph *g)
{ 
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

    // Create badji file
    char filename[FILENAMELEN];
    strcpy(filename, g->filename);
    strcat(filename, "i");
    FILE *out = (FILE *) fopen(filename, "w");

    // Check file stream
    if (out == NULL)
    {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }

    // Write number of blocks, block indices, and first nodes
    fwrite(&nblks, sizeof(unsigned long long), 1, out);
    fwrite(indices, sizeof(unsigned long long), nblks, out);
    fwrite(firstnodes, sizeof(unsigned int), nblks, out);

    // Close badji file
    fclose(out);

    // Free block
    free(block);

    return 0;
}

/* Get the next block of the graph. */
int nextblock(graph *g, unsigned int threadno)
{
    // Test for badji file
    if (!g->badji)
    {
        fprintf(stderr, "Graph must have a badji file.\n");
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
    // Test for badji file
    if (!g->badji)
    {
        fprintf(stderr, "Graph must have a badji file.\n");
        return 1;
    }

    // If there is no next node
    if ((g->currblockno[threadno] < g->nblks && g->currnode[threadno] == g->firstnodes[g->currblockno[threadno]]) || feof(g->currblock[threadno]))
    {
        return -1;
    }

    // Otherwise, read next node
    fread(&v->deg, sizeof(unsigned int), 1, g->currblock[threadno]);
    v->adj = malloc(v->deg * sizeof(unsigned int));
    fread(v->adj, sizeof(unsigned int), v->deg, g->currblock[threadno]);
    g->currnode[threadno]++;
    return (g->currnode[threadno] - 1);
}

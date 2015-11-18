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
        g->stream = (FILE *) fopen64(g->filename, "r");
    
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
        // Get directory name
        strcpy(g->filename, filename);
        
        // Open index file
        char indexfile[FILENAMELEN];
        strcpy(indexfile, g->filename);
        strcat(indexfile, "/0");
        g->stream = fopen(indexfile, "r");

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

        // Get first nodes
        g->firstnodes = malloc(g->nblks * sizeof(unsigned int));
        fread(g->firstnodes, sizeof(unsigned int), g->nblks, g->stream);

        // Get top nodes
        g->topnodes = malloc(TOPLEN * sizeof(unsigned int));
        fread(g->topnodes, sizeof(unsigned int), TOPLEN, g->stream);

        // Close index file
        fclose(g->stream);
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
        g->currblockfile[i] = NULL;
        g->currblockno[i] = i - NTHREADS + 1;
    }

    // Set number of threads
    omp_set_num_threads(NTHREADS);

    return 0;
}

/* Partition a BADJ graph into a BADJBLK graph. */
int partition(graph *g, char *dirname)
{
    // Declare variables
    unsigned int nblocks = 0;
    unsigned int currnode = 0;
    unsigned int firstnodes[MAXBLKS];
    unsigned int *nextblock = malloc(BLOCKLEN);
    char filename[FILENAMELEN];
    FILE *out;
    
    // Initialize indegrees
    unsigned int *indegrees = malloc(sizeof(unsigned int) * g->n);
    unsigned int i;
    for (i = 0; i < g->n; i++)
    {
        indegrees[i] = 0;
    }
    
    // Test graph format
    if (g->format != BADJ)
    {
        fprintf(stderr, "Original graph must be in BADJ format.\n");
        return 1;
    }

    // For each block
    while (1)
    {
        // Read maximal next block
        unsigned int intsread = fread(nextblock, sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);
        unsigned int nextblocklen = 0;
        
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

        // Create file for next block
        char blockno[128];
        sprintf(blockno, "%d", nblocks);
        strcpy(filename, dirname);
        strcat(filename, "/");
        strcat(filename, blockno);
        out = fopen(filename, "w");

        // Check file stream
        if (out == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return 1;
        }

        // Set first node in block
        firstnodes[nblocks-1] = currnode;

        // For each node
        while (1)
        {
            // Get degree
            unsigned int val = nextblock[nextblocklen];

            // Test whether node fits in next block
            if (nextblocklen + 1 + val <= intsread)
            {
                // If so, update indegrees
                unsigned int j;
                for (j = 0; j < val; j++)
                {
                    indegrees[nextblock[nextblocklen + 1 + j]]++;
                }
                
                // Increment next block length
                nextblocklen = nextblocklen + 1 + val;
                currnode++;
            }
            else
            {
                // Otherwise, unread node
                int extra = intsread - nextblocklen;
                fseek(g->stream, -extra*sizeof(unsigned int), SEEK_CUR);
                break;
            }
        }
        
        // Write next block to file
        fwrite(nextblock, sizeof(unsigned int), nextblocklen, out);
        fclose(out);
    }

    // Free next block
    free(nextblock);

    // Find top-indegree nodes
    unsigned int topnodes[MAXTOPLEN];
    unsigned int j;
    for (j = 0; j < MAXTOPLEN; j++)
    {
        topnodes[j] = (unsigned int) -1;
    }
    for (i = 0; i < g->n; i++)
    {
        for (j = 0; j < MAXTOPLEN; j++)
        {
            if ((topnodes[j] == (unsigned int) -1) || indegrees[i] > indegrees[topnodes[j]])
            {
                topnodes[j] = i;
                break;
            }
        }
    }
    free(indegrees);

    // Create index file
    strcpy(filename, dirname);
    strcat(filename, "/0");
    out = fopen(filename, "w");

    // Check file stream
    if (out == NULL)
    {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }

    // Write numbers of nodes, edges, and blocks to index file
    unsigned long long nblocksl = (unsigned long long) nblocks;
    fwrite(&g->n, sizeof(unsigned long long), 1, out);
    fwrite(&g->m, sizeof(unsigned long long), 1, out);
    fwrite(&nblocksl, sizeof(unsigned long long), 1, out);
    
    // Write first nodes to index file
    fwrite(firstnodes, sizeof(unsigned int), nblocks, out);

    // Write top-indegree nodes to index file
    fwrite(topnodes, sizeof(unsigned int), MAXTOPLEN, out);

    // Close index file
    fclose(out);

    return 0;
}

/* Transpose a BADJ graph. */
int transpose(graph *g, char *filename)
{
    // Declare variables
    unsigned int i;
    unsigned int j;
    unsigned int deg;
    unsigned int *adj;
    unsigned int *degt;
    unsigned int *currdegt;
    unsigned int **adjt;
    FILE *out;

    // Test graph format
    if (g->format != BADJ)
    {
        fprintf(stderr, "Original graph must be in BADJ format.\n");
        return 1;
    }

    // Allocate space for degrees in transpose
    degt = malloc(sizeof(unsigned int) * g->n);
    currdegt = malloc(sizeof(unsigned int) * g->n);
    for (i = 0; i < g->n; i++)
    {
        degt[i] = 0;
        currdegt[i] = 0;
    }

    // For each node
    for (i = 0; i < g->n; i++)
    {
        // Read adjacency list
        fread(&deg, sizeof(unsigned int), 1, g->stream);
        adj = malloc(sizeof(unsigned int) * deg);
        fread(adj, sizeof(unsigned int), deg, g->stream);

        // Increment degrees in transpose
        for (j = 0; j < deg; j++)
        {
            degt[adj[j]]++;
        }
        free(adj);
    }

    // Allocate space for adjacency list in transpose
    adjt = malloc(sizeof(unsigned int *) * g->n);
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
        for (j = 0; j < deg; j++)
        {
            adjt[adj[j]][currdegt[adj[j]]] = i;
            currdegt[adj[j]]++;
        }
        free(adj);
    }

    // Create BADJ file
    out = fopen(filename, "w");

    // Write number of nodes and edges
    fwrite(&g->n, sizeof(unsigned long long), 1, out);
    fwrite(&g->m, sizeof(unsigned long long), 1, out);
    
    // For each node
    for (i = 0; i < g->n; i++)
    {
        // Write degree in transpose
        fwrite(&degt[i], sizeof(unsigned int), 1, out);

        // Write each adjacent node in transpose
        for (j = 0; j < degt[i]; j++)
        {
            fwrite(&adjt[i][j], sizeof(unsigned int), 1, out);
        }
    }

    // Close BADJ file
    fclose(out);
    
    // Free variables
    free(degt);
    free(currdegt);
    free(adjt);

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

    // Open new block file
    if (g->currblockfile[threadno] != NULL)
    {
        fclose(g->currblockfile[threadno]);
    }
    char blockfile[FILENAMELEN];
    char blockno[128];
    sprintf(blockno, "%d", g->currblockno[threadno]);
    strcpy(blockfile, g->filename);
    strcat(blockfile, "/");
    strcat(blockfile, blockno);
    g->currblockfile[threadno] = fopen(blockfile, "r");

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
    unsigned int intsread = fread(&v->deg, sizeof(unsigned int), 1, g->currblockfile[threadno]);

    if (intsread > 0)
    {
        // Read next node
        v->adj = malloc(v->deg * sizeof(unsigned int));
        fread(v->adj, sizeof(unsigned int), v->deg, g->currblockfile[threadno]);
        g->currnode[threadno]++;
        return (g->currnode[threadno] - 1);
    }
    
    // No next node
    return -1;
}

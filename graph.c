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
        g->currblock[i] = malloc(BLOCKLEN);
        g->currblocklen[i] = 0;
        g->currblocki[i] = 0;

        // Next blocks
        g->nextblock[i] = malloc(BLOCKLEN);
        g->nextblocklen[i] = 0;
        g->nextblockno[i] = i + 1;
    }

    // Initialize thread attributes
    pthread_attr_init(&g->attr);
    pthread_attr_setdetachstate(&g->attr, PTHREAD_CREATE_JOINABLE);

    return 0;
}

/* Partition a BADJ graph into a BADJBLK graph. */
int partition(graph *g, char *dirname)
{
    // Declare variables
    FILE *out;
    char filename[FILENAMELEN];
    char blockno[128];
    unsigned int nblocks = 0;
    unsigned int intsread;
    unsigned int val;
    unsigned int currnode = 0;
    unsigned int firstnodes[MAXBLKS];

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
        intsread = fread(g->nextblock[0], sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);
        g->nextblocklen[0] = 0;
        
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
            val = g->nextblock[0][g->nextblocklen[0]];

            // Test whether node fits in next block
            if (g->nextblocklen[0] + 1 + val <= intsread)
            {
                // If so, increment next block length
                g->nextblocklen[0] = g->nextblocklen[0] + 1 + val;
                currnode++;
            }
            else
            {
                // Otherwise, unread node
                int extra = intsread - g->nextblocklen[0];
                fseek(g->stream, -extra*sizeof(unsigned int), SEEK_CUR);
                break;
            }
        }
        
        // Write next block to file
        fwrite(g->nextblock[0], sizeof(unsigned int), g->nextblocklen[0], out);
        fclose(out);
    }

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
    
    // Close index file
    fclose(out);

    return 0;
}

/* Read the next blocks of the graph. */
void *loadblocks(void *vg)
{
    // Get argument
    graph *g = (graph *) vg;
    
    // Test graph format
    if (g->format != BADJBLK)
    {
        fprintf(stderr, "Graph must be in BADJBLK format.\n");
        return NULL;
    }

    // Declare variables
    char graphfile[FILENAMELEN];
    char blockno[128];

    // For each next block
    unsigned int i;
    for (i = 0; i < NTHREADS; i++)
    {
        // Test whether there is a next block
        if (g->nextblockno[i] > g->nblks)
        {
            g->nextblocklen[i] = 0;
            continue;
        }

        // Open next block file
        sprintf(blockno, "%d", g->nextblockno[i]);
        strcpy(graphfile, g->filename);
        strcat(graphfile, "/");
        strcat(graphfile, blockno);
        g->stream = fopen(graphfile, "r");

        // Check file stream
        if (g->stream == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return NULL;
        }

        // Read next block
        g->nextblocklen[i] = fread(g->nextblock[i], sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);

        // Close next block file
        fclose(g->stream);
    }

    return NULL;
}

/* Get the next two blocks of the graph. */
int nextblocks(graph *g)
{
    // Declare variables
    unsigned int *tmp;

    // Test graph format
    if (g->format != BADJBLK)
    {
        fprintf(stderr, "Graph must be in BADJBLK format.\n");
        return 1;
    }

    // For each block
    unsigned int i;
    for (i = 0; i < NTHREADS; i++)
    {
        // Get block
        tmp = g->currblock[i];
        g->currblock[i] = g->nextblock[i];
        g->nextblock[i] = tmp;
        g->currblocklen[i] = g->nextblocklen[i];
        g->currblockno[i] = g->nextblockno[i];
        g->currblocki[i] = 0;
        g->currnode[i] = g->firstnodes[g->currblockno[i] - 1];

        // Set next block number
        g->nextblockno[i] = g->nextblockno[i] + NTHREADS;
    }

    // Fix up next block numbers
    if (g->nextblockno[0] > g->nblks)
    {
        for (i = 0; i < NTHREADS; i++)
        {
            g->nextblockno[i] = i + 1;
        }
    }

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

    // Test whether the block has more nodes
    if (g->currblocki[threadno] < g->currblocklen[threadno])
    {
        // Get next node
        v->deg = g->currblock[threadno][g->currblocki[threadno]];
        g->currblocki[threadno]++;
        v->adj = g->currblock[threadno] + g->currblocki[threadno];
        g->currblocki[threadno] = g->currblocki[threadno] + v->deg;
        g->currnode[threadno]++;
        return (g->currnode[threadno] - 1);
    }
    else
    {
        // No next node;
        return -1;
    }

    return 0;
}

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

    // Initialize current block 1
    g->currblock1 = malloc(BLOCKLEN);
    g->currblocklen1 = 0;
    g->currblocki1 = 0;

    // Initialize current block 2
    g->currblock2 = malloc(BLOCKLEN);
    g->currblocklen2 = 0;
    g->currblocki2 = 0;

    // Initialize next block 1
    g->nextblock1 = malloc(BLOCKLEN);
    g->nextblocklen1 = 0;
    g->nextblockno1 = 1;
    
    // Initialize next block 2
    g->nextblock2 = malloc(BLOCKLEN);
    g->nextblocklen2 = 0;
    g->nextblockno2 = 2;
 
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
        intsread = fread(g->nextblock1, sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);
        g->nextblocklen1 = 0;
        
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
            val = g->nextblock1[g->nextblocklen1];

            // Test whether node fits in next block
            if (g->nextblocklen1 + 1 + val <= intsread)
            {
                // If so, increment next block length
                g->nextblocklen1 = g->nextblocklen1 + 1 + val;
                currnode++;
            }
            else
            {
                // Otherwise, unread node
                int extra = intsread - g->nextblocklen1;
                fseek(g->stream, -extra*sizeof(unsigned int), SEEK_CUR);
                break;
            }
        }
        
        // Write next block to file
        fwrite(g->nextblock1, sizeof(unsigned int), g->nextblocklen1, out);
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

/* Read the next two blocks of the graph. */
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

    char graphfile[FILENAMELEN];
    char blockno[128];
        
    // Open next block file 1
    sprintf(blockno, "%d", g->nextblockno1);
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

    // Read next block 1
    g->nextblocklen1 = fread(g->nextblock1, sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);
    g->nextnode1 = g->firstnodes[g->nextblockno1];

    // Close next block file 1
    fclose(g->stream);

    // Test whether there is a next block 2
    if (g->nextblockno2 <= g->nblks)
    {
        // Open next block file 2
        sprintf(blockno, "%d", g->nextblockno2);
        strcpy(graphfile, g->filename);
        strcat(graphfile, "/");
        strcat(graphfile, blockno);
        g->stream = fopen(graphfile, "r");

        // Check file stream
        if (g->stream == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
        }

        // Read next block 2
        g->nextblocklen2 = fread(g->nextblock2, sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);
        g->nextnode2 = g->firstnodes[g->nextblockno2];

        // Close next block file 2
        fclose(g->stream);
    }
    else
    {
        // There is no next block 2
        g->nextblocklen2 = 0;
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

    // Get block 1
    tmp = g->currblock1;
    g->currblock1 = g->nextblock1;
    g->nextblock1 = tmp;
    g->currblocklen1 = g->nextblocklen1;
    g->currblockno1 = g->nextblockno1;
    g->currblocki1 = 0;
    g->currnode1 = g->nextnode1;

    // Get block 2
    tmp = g->currblock2;
    g->currblock2 = g->nextblock2;
    g->nextblock2 = tmp;
    g->currblocklen2 = g->nextblocklen2;
    g->currblockno2 = g->nextblockno2;
    g->currblocki2 = 0;
    g->currnode2 = g->nextnode2;

    // Set next block numbers
    g->nextblockno1 = g->nextblockno1 + 2;
    g->nextblockno2 = g->nextblockno2 + 2;
    if (g->nextblockno1 > g->nblks)
    {
        g->nextblockno1 = 1;
        g->nextblockno2 = 2;
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

    // Test whether this is computation thread 1 or 2
    if (threadno == 1)
    {
        // Test whether the block has more nodes
        if (g->currblocki1 < g->currblocklen1)
        {
            // Get next node
            v->deg = g->currblock1[g->currblocki1];
            g->currblocki1++;
            v->adj = g->currblock1 + g->currblocki1;
            g->currblocki1 = g->currblocki1 + v->deg;
            g->currnode1++;
            return (g->currnode1 - 1);
        }
        else
        {
            // No next node
            return -1;
        }
    }
    else if (threadno == 2)
    {
        // Test whether the block has more nodes
        if (g->currblocki2 < g->currblocklen2)
        {
            // Get next node
            v->deg = g->currblock2[g->currblocki2];
            g->currblocki2++;
            v->adj = g->currblock2 + g->currblocki2;
            g->currblocki2 = g->currblocki2 + v->deg;
            g->currnode2++;
            return (g->currnode2 - 1);
        }
        else
        {
            // No next node
            return (unsigned int) -1;
        }
    } 

    return 0;
}

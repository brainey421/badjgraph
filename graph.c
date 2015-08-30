#include "graph.h"

int initialize(graph *g, char *filename, char format)
{
    if (strlen(filename) > FILENAMELEN)
    {
        fprintf(stderr, "Max file name length exceeded.\n");
        return 1;
    }

    g->format = format;
    
    if (g->format == BADJ)
    {
        strcpy(g->filename, filename);    
        g->stream = (FILE *) fopen64(g->filename, "r");
    
        if (g->stream == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return 1;
        }

        fread(&g->n, sizeof(unsigned long long), 1, g->stream);
        fread(&g->m, sizeof(unsigned long long), 1, g->stream);
    }
    else if (g->format == BADJBLK)
    {
        strcpy(g->filename, filename);
        
        char indexfile[FILENAMELEN];
        strcpy(indexfile, g->filename);
        strcat(indexfile, "/0");
        g->stream = fopen(indexfile, "r");

        if (g->stream == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return 1;
        }

        fread(&g->n, sizeof(unsigned long long), 1, g->stream);
        fread(&g->m, sizeof(unsigned long long), 1, g->stream);
        fread(&g->nblks, sizeof(unsigned long long), 1, g->stream);

        fclose(g->stream);
    }
    
    if (g->m > 4294967296)
    {
        fprintf(stderr, "Too many edges to handle: %llu\n", g->m);
        return 1;
    }

    g->currblock = malloc(BLOCKLEN);
    g->currblocklen = 0;
    g->currblocki = 0;

    g->nextblock = malloc(BLOCKLEN);
    g->nextblocklen = 0;
    g->nextblockno = 1;
 
    pthread_attr_init(&g->attr);
    pthread_attr_setdetachstate(&g->attr, PTHREAD_CREATE_JOINABLE);

    return 0;
}

int partition(graph *g, char *dirname)
{
    FILE *out;
    char filename[FILENAMELEN];
    char blockno[128];
    unsigned int nblocks = 0;
    unsigned int intsread;
    unsigned int val;

    while (1)
    {
        intsread = fread(g->nextblock, sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);
        g->nextblocklen = 0;
        
        if (intsread == 0)
        {
            break;
        }
        
        nblocks++;
        sprintf(blockno, "%d", nblocks);
        strcpy(filename, dirname);
        strcat(filename, "/");
        strcat(filename, blockno);
        out = fopen(filename, "w");

        if (out == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return 1;
        }

        while (1)
        {
            val = g->nextblock[g->nextblocklen];

            if (g->nextblocklen + 1 + val <= intsread)
            {
                g->nextblocklen = g->nextblocklen + 1 + val;
            }
            else
            {
                int extra = intsread - g->nextblocklen;
                fseek(g->stream, -extra*sizeof(unsigned int), SEEK_CUR);
                break;
            }
        }
        
        fwrite(g->nextblock, sizeof(unsigned int), g->nextblocklen, out);
        fclose(out);
    }

    strcpy(filename, dirname);
    strcat(filename, "/0");
    out = fopen(filename, "w");

    if (out == NULL)
    {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }

    unsigned long long nblocksl = (unsigned long long) nblocks;
    fwrite(&g->n, sizeof(unsigned long long), 1, out);
    fwrite(&g->m, sizeof(unsigned long long), 1, out);
    fwrite(&nblocksl, sizeof(unsigned long long), 1, out);
    fclose(out);

    return 0;
}

void *loadblock(void *vg)
{
    graph *g = (graph *) vg;
    
    if (g->format == BADJ)
    {
        unsigned int intsread;
        intsread = fread(g->nextblock, sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);
        g->nextblocklen = 0;

        if (feof(g->stream))
        {
            fseek(g->stream, 2*sizeof(unsigned long long), SEEK_SET);
        }

        unsigned int val;
        while (1)
        {
            val = g->nextblock[g->nextblocklen];

            if (g->nextblocklen + 1 + val <= intsread)
            {
                g->nextblocklen = g->nextblocklen + 1 + val;
            }
            else
            {
                int extra = intsread - g->nextblocklen;
                fseek(g->stream, -extra*sizeof(unsigned int), SEEK_CUR);
                break;
            }
        }
    }
    else if (g->format == BADJBLK)
    {
        char graphfile[FILENAMELEN];
        char blockno[128];
        sprintf(blockno, "%d", g->nextblockno);
        strcpy(graphfile, g->filename);
        strcat(graphfile, "/");
        strcat(graphfile, blockno);
        g->stream = fopen(graphfile, "r");

        if (g->stream == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return NULL;
        }

        g->nextblocklen = fread(g->nextblock, sizeof(unsigned int), BLOCKLEN / sizeof(unsigned int), g->stream);

        fclose(g->stream);
        
        g->nextblockno = g->nextblockno + 1;
        if (g->nextblockno > g->nblks)
        {
            g->nextblockno = 1;
        }
    }

    return NULL;
}

int nextnode(graph *g, node *v)
{
    if (g->format == BADJ || g->format == BADJBLK)
    {
        if (g->currblocklen == 0)
        {
            pthread_create(&g->reader, &g->attr, loadblock, (void *) g);
            pthread_join(g->reader, NULL);

            unsigned int *tmp = g->currblock;
            g->currblock = g->nextblock;
            g->nextblock = tmp;
            g->currblocklen = g->nextblocklen;
            g->currblocki = 0;
        
            pthread_create(&g->reader, &g->attr, loadblock, (void *) g);
        }

        if (g->currblocki >= g->currblocklen)
        {
            pthread_join(g->reader, NULL);

            unsigned int *tmp = g->currblock;
            g->currblock = g->nextblock;
            g->nextblock = tmp;
            g->currblocklen = g->nextblocklen;
            g->currblocki = 0;
        
            pthread_create(&g->reader, &g->attr, loadblock, (void *) g);
        }

        v->deg = g->currblock[g->currblocki];
        g->currblocki++;
        v->adj = g->currblock + g->currblocki;
        g->currblocki += v->deg;
    }

    return 0;
}

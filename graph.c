#include "graph.h"

int initialize(graph *g, char *filename, char format)
{
    if (strlen(filename) > FILENAMELEN)
    {
        fprintf(stderr, "Max file name length exceeded.\n");
        return 1;
    }

    strcpy(g->filename, filename);
    
    g->stream = (FILE *) fopen64(g->filename, "r");

    if (g->stream == NULL)
    {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }
    
    g->format = format;

    if (g->format == SMAT)
    {
        char buff[BUFFSIZE];
    
        fscanf(g->stream, "%s", buff);
        fscanf(g->stream, "%s", buff);
        g->n = strtoull(buff, NULL, 0);
    
        fscanf(g->stream, "%s", buff);
        g->m = strtoull(buff, NULL, 0);
    }
    else if (g->format == BSMAT)
    {
        int n, m;

        fread(&n, sizeof(int), 1, g->stream);
        fread(&n, sizeof(int), 1, g->stream);
        fread(&m, sizeof(int), 1, g->stream);

        g->n = (unsigned long long) n;
        g->m = (unsigned long long) m;
    }
    else
    {
        fread(&g->n, sizeof(unsigned long long), 1, g->stream);
        fread(&g->m, sizeof(unsigned long long), 1, g->stream);
    }

    if (g->m > 4294967296)
    {
        fprintf(stderr, "Too many edges to handle: %llu\n", g->m);
        return 1;
    }

    recentedge.src = -1;

    return 0;
}

int nextedge(graph *g, edge *e)
{
    if (feof(g->stream))
    {
        return 1;
    }
   
    if (g->format == SMAT)
    {
        char buff[BUFFSIZE];
    
        fscanf(g->stream, "%s", buff);
        e->src = atoi(buff);
    
        fscanf(g->stream, "%s", buff);
        e->dest = atoi(buff);

        fscanf(g->stream, "%s", buff);
    }
    else if (g->format == BSMAT)
    {
        double weight;
        unsigned long long val;
        
        fread(&val, sizeof(unsigned long long), 1, g->stream);
        e->src = (unsigned int) val;
        fread(&val, sizeof(unsigned long long), 1, g->stream);
        e->dest = (unsigned int) val;

        fread(&weight, sizeof(double), 1, g->stream);
    }
    else
    {
        return 1;
    }

    return 0;
}

int nextnode(graph *g, node *v, unsigned int i)
{
    if (g->format == SMAT || g->format == BSMAT)
    {
        unsigned int size = 64;

        v->deg = 0;
        v->adj = malloc(size*sizeof(unsigned int));

        edge e;

        while (1)
        {
            if (v->deg == 0 && recentedge.src == i)
            {
                e = recentedge;
            }
            else if (recentedge.src != -1 && recentedge.src > i)
            {
                break;
            }
            else if (nextedge(g, &e))
            {
                recentedge.src = -1;
                break;
            }
            else if (e.src != i)
            {
                recentedge = e;
                break;
            }
        
            if (v->deg == size)
            {
                size = 2*size;
                unsigned int *tmp = malloc(size*sizeof(unsigned int));
                unsigned int j;
                for (j = 0; j < size / 2; j++)
                {
                    tmp[j] = v->adj[j];
                }
                free(v->adj);
                v->adj = tmp;
            }

            v->adj[v->deg] = e.dest;
            v->deg++;
        }
    }
    else
    {
        fread(&v->deg, sizeof(unsigned int), 1, g->stream);
        v->adj = malloc(v->deg*sizeof(unsigned int));       
        fread(v->adj, sizeof(unsigned int), v->deg, g->stream);
    }

    return 0;
}

int rewindedges(graph *g)
{
    if (g->format == SMAT)
    {
        fseek(g->stream, 0, SEEK_SET);
        
        char buff[BUFFSIZE];

        fscanf(g->stream, "%s", buff);
        fscanf(g->stream, "%s", buff);
        fscanf(g->stream, "%s", buff);
    }
    else if (g->format == BSMAT)
    {
        fseek(g->stream, 3*sizeof(int), SEEK_SET);
    }
    else
    {
        fseek(g->stream, 2*sizeof(unsigned long long), SEEK_SET);
    }

    return 0;
}

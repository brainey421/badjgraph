#include "graph.h"

int initialize(graph *g, char *filename, char format)
{
    if (strlen(filename) > FILENAMELEN)
    {
        return 1;
    }

    strcpy(g->filename, filename);
    g->stream = fopen(g->filename, "r");
    
    if (g->stream == NULL)
    {
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
        unsigned long long n;
        unsigned long long m;

        fread(&n, sizeof(unsigned long long), 1, g->stream);
        fread(&m, sizeof(unsigned long long), 1, g->stream);

        g->n = n;
        g->m = m;
    }

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
        e->src = strtoull(buff, NULL, 0);
    
        fscanf(g->stream, "%s", buff);
        e->dest = strtoull(buff, NULL, 0);

        fscanf(g->stream, "%s", buff);
        e->weight = atoi(buff);
    }
    else if (g->format == BSMAT)
    {
        double weight;

        fread(&e->src, sizeof(unsigned long long), 1, g->stream);
        fread(&e->dest, sizeof(unsigned long long), 1, g->stream);
        fread(&weight, sizeof(double), 1, g->stream);

        e->weight = 1;
    }
    else
    {
        return 1;
    }

    return 0;
}

int nextnode(graph *g, node *v, unsigned long long i)
{
    if (g->format != BADJ)
    {
        unsigned long long size = 64;

        v->deg = 0;
        v->adj = malloc(size*sizeof(unsigned long long));

        edge e;

        while (!nextedge(g, &e))
        {
            if (e.src != i)
            {
                break;
            }
        
            if (v->deg == size)
            {
                size = 2*size;
                unsigned long long *tmp = malloc(size*sizeof(unsigned long long));
                unsigned long long j;
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

        fseek(g->stream, -1*(2*sizeof(unsigned long long) + sizeof(double)), SEEK_CUR);
    }
    else
    {
        unsigned long long deg;
        fread(&deg, sizeof(unsigned long long), 1, g->stream);
        v->deg = deg;

        v->adj = malloc(deg*sizeof(unsigned long long));

        unsigned long long j;
        for (j = 0; j < deg; j++)
        {
            fread(v->adj + j, sizeof(unsigned long long), 1, g->stream);
        }
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

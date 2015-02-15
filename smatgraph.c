#include "smatgraph.h"

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
    else
    {
        int n, m;

        fread(&n, sizeof(int), 1, g->stream);
        fread(&n, sizeof(int), 1, g->stream);
        fread(&m, sizeof(int), 1, g->stream);

        g->n = (unsigned long long) n;
        g->m = (unsigned long long) m;
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
    else
    {
        double weight;

        fread(&e->src, sizeof(unsigned long long), 1, g->stream);
        fread(&e->dest, sizeof(unsigned long long), 1, g->stream);
        fread(&weight, sizeof(double), 1, g->stream);

        e->weight = 1;
    }

    return 0;
}

int nextnode(graph *g, node *v)
{
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
    else
    {
        fseek(g->stream, 3*sizeof(int), SEEK_SET);
    }

    return 0;
}

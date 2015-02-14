#include "smatgraph.h"

int initialize(graph *g, char *filename)
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
    
    char buff[BUFFSIZE];
    
    fscanf(g->stream, "%s", buff);
    fscanf(g->stream, "%s", buff);
    g->n = strtoull(buff, NULL, 0);
    
    fscanf(g->stream, "%s", buff);
    g->m = strtoull(buff, NULL, 0);

    if (g->n == 0 || g->m == 0)
    {
        return 1;
    }

    return 0;
}

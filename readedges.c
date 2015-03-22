#include "graph.h"

/* Purely reads through the edge file of a directed graph in SMAT, BSMAT,
 * BADJ, or BADJGZ format. */
int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: ./readedges [graphfile] [smat|bsmat|badj|badjgz] [niterations]\n");
        return 1;
    }
    
    char format;
    if (!strcmp(argv[2], "bsmat"))
    {
        format = BSMAT;
    }
    else if (!strcmp(argv[2], "smat"))
    {
        format = SMAT;
    }
    else if (!strcmp(argv[2], "badj"))
    {
        format = BADJ;
    }
    else if (!strcmp(argv[2], "badjgz"))
    {
        format = BADJGZ;
    }
    else
    {
        fprintf(stderr, "Unknown format.\n");
        return 1;
    }

    graph g;
    if (initialize(&g, argv[1], format))
    {
        return 1;
    }

    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    unsigned long long niter = (unsigned long long) atoi(argv[3]);
    unsigned long long i, j;
    node v;
    for (i = 0; i < niter; i++)
    {
        fprintf(stderr, "%llu\n", i);

        for (j = 0; j < g.n; j++)
        {
            nextnode(&g, &v, j);
            free(v.adj);
        }
        
        rewindedges(&g);
    }

    return 0;
}
#include "graph.h"

/* Purely reads through the edge file of a directed graph in
 * BADJ format. */
int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: ./readedges [graphfile] [badj|badjblk] [niterations]\n");
        return 1;
    }
    
    // TODO

    /*
    char format;
    if (!strcmp(argv[2], "badj"))
    {
        format = BADJ;
    }
    else if (!strcmp(argv[2], "badjblk"))
    {
        format = BADJBLK;
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

    unsigned int niter = (unsigned int) atoi(argv[3]);
    unsigned int i, j;
    node v;
    for (i = 0; i < niter; i++)
    {
        fprintf(stderr, "%u\n", i);

        for (j = 0; j < g.n; j++)
        {
            nextnode(&g, &v);
        }
    }
    */

    return 0;
}

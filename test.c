#include "smatgraph.h"

int main()
{
    graph g;
    initialize(&g, "/media/drive/graphs/cnr-2000.smat");

    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    unsigned long long counter = 0;
    edge e;
    while (1)
    {
        if (nextedge(&g, &e))
        {
            break;
        }
        counter++;
        if (counter % 100000 == 0)
        {
            fprintf(stderr, "Edge: %llu %llu %d\n", e.src, e.dest, e.weight);
        }
    }

    rewindedges(&g);

    while (1)
    {
        if (nextedge(&g, &e))
        {
            break;
        }
        counter++;
        if (counter % 100000 == 0)
        {
            fprintf(stderr, "Edge: %llu %llu %d\n", e.src, e.dest, e.weight);
        }
    }

    fprintf(stderr, "\nEdges: %llu\n", counter);

    return 0;
}

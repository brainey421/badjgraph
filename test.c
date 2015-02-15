#include "smatgraph.h"

int main()
{
    graph g;
    initialize(&g, "/media/drive/graphs/cnr-2000.bsmat", BSMAT);

    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    edge e;
    
    nextedge(&g, &e);
    nextedge(&g, &e);
    nextedge(&g, &e);
    nextedge(&g, &e);
    nextedge(&g, &e);
    nextedge(&g, &e);
    nextedge(&g, &e);
    fprintf(stderr, "Seventh edge: %llu, %llu, %d\n", e.src, e.dest, e.weight);
    
    while (1)
    {
        if (nextedge(&g, &e))
        {
            break;
        }
    }

    rewindedges(&g);

    nextedge(&g, &e);
    nextedge(&g, &e);
    nextedge(&g, &e);
    nextedge(&g, &e);
    nextedge(&g, &e);
    nextedge(&g, &e);
    nextedge(&g, &e);
    fprintf(stderr, "Seventh edge: %llu, %llu, %d\n", e.src, e.dest, e.weight);
 
    while (1)
    {
        if (nextedge(&g, &e))
        {
            break;
        }
    }

    return 0;
}

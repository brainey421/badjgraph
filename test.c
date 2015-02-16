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

    node v;
    unsigned long long i = 0;
    for (i = 0; i < g.n; i++)
    {
        nextnode(&g, &v, i);

        if (i % 10000 == 0)
        {
            fprintf(stderr, "Node %llu has degree %llu ... ", i, v.deg);
            unsigned long long j = 0;
            for (j = 0; j < v.deg; j++)
            {
                fprintf(stderr, "%llu ", v.adj[j]);
            }
            fprintf(stderr, "\n");
        }

        free(v.adj);
    }
    
    return 0;
}

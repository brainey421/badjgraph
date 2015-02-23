#include "graph.h"

int main()
{
    graph g;
    initialize(&g, "/media/drive/graphs/cnr-2000.badj", BADJ);

    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

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

#include "smatgraph.h"

int main()
{
    graph g;
    initialize(&g, "/media/drive/graphs/cnr-2000.smat");
    
    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n", g.m);

    return 0;
}

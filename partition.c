#include "graph.h"

/* Partition a BADJ graph into a BADJBLK graph. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 2)
    {
        fprintf(stderr, "Usage: ./partition [BADJ file]\n");
        return 1;
    }
    
    // Initialize graph
    graph g;
    if (initialize(&g, argv[1], BADJ))
    {
        return 1;
    }

    // Print numbers of nodes and edges
    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    // Partition graph
    partition(&g);

    // Destroy graph
    destroy(&g);

    return 0;
}

#include "graph.h"

/* Transposes a BADJ graph. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./transpose [BADJ file] [transposed BADJ file]\n");
        return 1;
    }
    
    // Initialize graph
    graph g;
    if (initialize(&g, argv[1], 0))
    {
        return 1;
    }

    // Print numbers of nodes and edges
    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    // Transpose graph
    transpose(&g, argv[2]);

    // Destroy graph
    destroy(&g);

    return 0;
}

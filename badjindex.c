#include "graph.h"

/* Create a badji file for a BADJ graph. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 2)
    {
        fprintf(stderr, "Usage: ./badjindex [BADJ file]\n");
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

    // Create badji file
    badjindex(&g);

    // Destroy graph
    destroy(&g);

    return 0;
}

#include "graph.h"

/* Computes the locality of a BADJ graph. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./locality [BADJ file] [window]\n");
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

    // Compute locality
    unsigned int window = (unsigned int) atoi(argv[2]);
    double loc;
    locality(&g, window, &loc);
    fprintf(stderr, "Locality: %e\n", loc);

    // Destroy graph
    destroy(&g);

    return 0;
}

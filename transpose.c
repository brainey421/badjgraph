#include "graph.h"

/* Transpose a BADJ graph into a BADJTBLK graph. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 4)
    {
        fprintf(stderr, "Usage: ./transpose [graphfile] badj [outdirectory]\n");
        return 1;
    }
    
    // Verify BADJ format
    char format;
    if (!strcmp(argv[2], "badj"))
    {
        format = BADJ;
    }
    else
    {
        fprintf(stderr, "Unknown format.\n");
        return 1;
    }

    // Initialize graph
    graph g;
    if (initialize(&g, argv[1], format))
    {
        return 1;
    }

    // Print numbers of nodes and edges
    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    // Transpose graph
    transpose(&g, argv[3]);

    return 0;
}

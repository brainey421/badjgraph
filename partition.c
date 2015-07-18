#include "graph.h"

/* Partition BADJ file into blocks. */
int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: ./partition [graphfile] [badj] [outdirectory]\n");
        return 1;
    }
    
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

    graph g;
    if (initialize(&g, argv[1], format))
    {
        return 1;
    }

    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    partition(&g, argv[3]);

    return 0;
}

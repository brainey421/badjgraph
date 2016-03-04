#include "graph.h"

/* Streams a BADJ graph. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 2)
    {
        fprintf(stderr, "Usage: ./stream [BADJ file]\n");
        return 1;
    }
    
    // Initialize graph
    graph g;
    if (initialize(&g, argv[1], 1))
    {
        return 1;
    }

    // Print numbers of nodes and edges
    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    // Get next blocks
    unsigned int i;
    for (i = 0; i < NTHREADS; i++)
    {
        nextblock(&g, i);
    }

    #pragma omp parallel
    {
        unsigned int threadno = omp_get_thread_num();
        while (1)
        {
            // Get the nodes
            while (1)
            {
                node v;
                unsigned int i = nextnode(&g, &v, threadno);
                if (i == (unsigned int) -1)
                {
                    break;
                }
                free(v.adj);
            }

            // Get the next block
            nextblock(&g, threadno);
            if (g.currblockno[threadno] <= NTHREADS)
            {
                break;
            }
        }
    }

    // Destroy graph
    destroy(&g);

    return 0;
}

#include "graph.h"

/* Perform Label Propagation. */
int propagate(graph *g, int maxit, unsigned int *x)
{
    // Initialize x to node numbers
    unsigned int i;
    for (i = 0; i < g->n; i++)
    {
        x[i] = i;
    }

    // Get next blocks
    for (i = 0; i < NTHREADS; i++)
    {
        nextblock(g, i);
    }

    // For each iteration
    unsigned int iter = 0;
    while (iter < maxit)
    {
        // Propagate labels
        unsigned int nprops = 0;
        #pragma omp parallel
        {
            unsigned int threadno = omp_get_thread_num();
            while (1)
            {
                while (1)
                {
                    // Get the next node
                    node v;
                    unsigned int i = nextnode(g, &v, threadno);
                    if (i == (unsigned int) -1)
                    {
                        break;
                    }

                    // Update neighbors
                    unsigned int j;
                    for (j = 0; j < v.deg; j++)
                    {
                        // NOT Thread Safe!!
                        unsigned int vadjj = v.adj[j];
                        if (x[i] < x[vadjj])
                        {
                            x[vadjj] = x[i];
                            nprops++;
                        }
                        else if (x[i] > x[vadjj])
                        {
                            x[i] = x[vadjj];
                            nprops++;
                        }
                    }
                    free(v.adj);
                }

                // Get next block
                nextblock(g, threadno);

                // Check if iteration is over
                if (g->currblockno[threadno] <= NTHREADS)
                {
                    break;
                }
            }
        }

        // Update number of iterations
        iter++;
        
        // Print number of propagations
        fprintf(stderr, "%d: %d\n", iter, nprops);
        
        // Stop iterating if no propagations
        if (nprops == 0)
        {
            break;
        }
    }

    return 0;
}

/* Computes the connected components of a graph in BADJBLK 
 * format using Label Propagation. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./components [BADJBLK file] [maxiter]\n");
        return 1;
    }
    
    // Initialize graph
    graph g;
    if (initialize(&g, argv[1], BADJBLK))
    {
        return 1;
    }

    // Print numbers of nodes and edges
    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    // Set number of iterations
    int maxit = atoi(argv[2]);

    // Initialize label vector
    unsigned int *x = malloc(g.n * sizeof(unsigned int));

    // Perform Label Propagation
    propagate(&g, maxit, x);

    // Test
    fprintf(stderr, "\n");
    unsigned int i;
    for (i = 0; i < 10; i++)
    {
        fprintf(stderr, "%d: %d\n", i, x[i]);
    }
   
    // Destroy label vector
    free(x);

    // Destroy graph
    destroy(&g);

    return 0;
}

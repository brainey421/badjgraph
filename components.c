#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/mman.h>
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

                    // Compute update for neighbors
                    unsigned int j;
                    for (j = 0; j < v.deg; j++)
                    {
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

                // Get the next block
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

/* Computes the connected components of a graph in BADJ 
 * format using Label Propagation. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./components [BADJ file] [maxiter] [optional out file]\n");
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

    // Set number of iterations
    int maxit = atoi(argv[2]);

    // Determine whether vector is out of core
    char ooc = 0;
    if (g.n > 134217728)
    {
        ooc = 1;
    }

    // Initialize label vector
    FILE *xfile;
    unsigned int *x;
    if (!ooc)
    {
        x = malloc(g.n * sizeof(unsigned int));
    }
    else
    {
        xfile = fopen("xfile.tmp", "w+");
        fallocate(fileno(xfile), 0, 0, g.n * sizeof(unsigned int));
        x = (unsigned int *) mmap(NULL, g.n * sizeof(unsigned int), PROT_READ|PROT_WRITE, MAP_SHARED, fileno(xfile), 0);
    }
    
    // Perform Label Propagation
    propagate(&g, maxit, x);

    // Optionally output x and destroy label vector
    if (!ooc)
    {
        if (argc > 3)
        {
            FILE *out = fopen(argv[3], "w");
            if (out == NULL)
            {
                fprintf(stderr, "Could not open output file.\n");
            }
            else
            {
                fwrite(x, sizeof(unsigned int), g.n, out);
                fclose(out);
            }
        }
        free(x);
    }
    else
    {
        if (argc > 3)
        {
            rename("xfile.tmp", argv[3]);
            fclose(xfile);
        }
        else
        {
            fclose(xfile);
            remove("xfile.tmp");
        }
    }
   
    // Destroy graph
    destroy(&g);

    return 0;
}

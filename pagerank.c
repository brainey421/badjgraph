#include "graph.h"

#define FPTYPE float

/* Perform one iteration of PowerIteration. */
int poweriterate(graph *g, FPTYPE alpha, FPTYPE *x, FPTYPE *y[])
{
    // Initialize y to 0
    unsigned int i;
    unsigned int j;
    //for (i = 0; i < NTHREADS; i++)
    //{
    //    for (j = 0; j < g->n; j++)
    //    {
    //        y[i][j] = 0.0;
    //    }
    //}
    for (j = 0; j < g->n; j++)
    {
        y[0][j] = 0.0;
    }

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

                // Compute
                if (v.deg != 0)
                {
                    FPTYPE update = alpha * x[i] / v.deg;
                    unsigned int j;
                    for (j = 7; j < v.deg; j += 8)
                    {
                        unsigned int vadj1 = v.adj[j-7];
                        unsigned int vadj2 = v.adj[j-6];
                        unsigned int vadj3 = v.adj[j-5];
                        unsigned int vadj4 = v.adj[j-4];
                        unsigned int vadj5 = v.adj[j-3];
                        unsigned int vadj6 = v.adj[j-2];
                        unsigned int vadj7 = v.adj[j-1];
                        unsigned int vadj8 = v.adj[j];
                        
                        #pragma omp atomic 
                        y[0][vadj1] += update;
                        #pragma omp atomic 
                        y[0][vadj2] += update;
                        #pragma omp atomic 
                        y[0][vadj3] += update;
                        #pragma omp atomic 
                        y[0][vadj4] += update;
                        #pragma omp atomic 
                        y[0][vadj5] += update;
                        #pragma omp atomic 
                        y[0][vadj6] += update;
                        #pragma omp atomic 
                        y[0][vadj7] += update;
                        #pragma omp atomic 
                        y[0][vadj8] += update;

                        //FPTYPE y1 = y[0][vadj1];
                        //FPTYPE y2 = y[0][vadj2];
                        //FPTYPE y3 = y[0][vadj3];
                        //FPTYPE y4 = y[0][vadj4];
                        //FPTYPE y5 = y[0][vadj5];
                        //FPTYPE y6 = y[0][vadj6];
                        //FPTYPE y7 = y[0][vadj7];
                        //FPTYPE y8 = y[0][vadj8];
                        
                        //y[0][vadj1] = y1 + update;
                        //y[0][vadj2] = y2 + update;
                        //y[0][vadj3] = y3 + update;
                        //y[0][vadj4] = y4 + update;
                        //y[0][vadj5] = y5 + update;
                        //y[0][vadj6] = y6 + update;
                        //y[0][vadj7] = y7 + update;
                        //y[0][vadj8] = y8 + update;
                    }
                    for (j = j - 7; j < v.deg; j++)
                    {
                        unsigned int vadjj = v.adj[j];
                        #pragma omp atomic 
                        y[0][vadjj] += update;
                        //FPTYPE yj = y[0][vadjj];
                        //y[0][vadjj] = yj + update;
                    }
                }
                free(v.adj);
            }

            // Get next block
            nextblock(g, threadno);

            fprintf(stderr, "%d\n", g->currblockno[threadno]);
            // Check if iteration is over
            if (g->currblockno[threadno] <= NTHREADS)
            {
                break;
            }
        }
    }

    // Add y vectors
    //for (i = 1; i < NTHREADS; i++)
    //{
    //    for (j = 0; j < g->n; j++)
    //    {
    //        y[0][j] += y[i][j];
    //    }
    //}
    
    // Distribute remaining weight among the nodes
    FPTYPE remainder = 1.0;
    for (i = 0; i < g->n; i++)
    {
        remainder -= y[0][i];
    }
    remainder /= (FPTYPE) g->n;
    for (i = 0; i < g->n; i++)
    {
        y[0][i] += remainder;
    }

    return 0;
}

/* Perform PowerIteration. */
int power(graph *g, FPTYPE alpha, FPTYPE tol, int maxit, FPTYPE *x, FPTYPE *y[])
{
    // Initialize x to e/n
    unsigned int i;
    FPTYPE init = 1.0 / (FPTYPE) g->n;
    for (i = 0; i < g->n; i++)
    {
        x[i] = init;
    }

    // Initialize number of iterations
    int iter = 0;

    // Get next blocks
    for (i = 0; i < NTHREADS; i++)
    {
        nextblock(g, i);
    }

    // For each iteration
    while (iter < maxit)
    {
        // Perform iteration
        poweriterate(g, alpha, x, y);
        iter++;
        
        // Compute residual norm
        FPTYPE norm = 0.0;
        for (i = 0; i < g->n; i++)
        {
            norm += fabs(x[i] - y[0][i]);
        }
        
        // Print residual norm
        fprintf(stderr, "%d: %e\n", iter, norm);
        
        // Copy y to x
        for (i = 0; i < g->n; i++)
        {
            x[i] = y[0][i];
        }

        // Stop iterating if residual norm is within tolerance
        if (norm < tol)
        {
            break;
        }
    }

    return 0;
}

/* Computes the PageRank vector of a directed graph in
 * BADJBLK format using PowerIteration or UpdateIteration. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 5)
    {
        fprintf(stderr, "Usage: ./pagerank [graphfile] badjblk power [maxiter]\n");
        return 1;
    }
    
    // Get graph format
    char format;
    if (!strcmp(argv[2], "badjblk"))
    {
        format = BADJBLK;
    }
    else
    {
        fprintf(stderr, "Unknown format.\n");
        return 1;
    }

    // Declare variables
    unsigned int i;
    
    // Initialize graph
    graph g;
    if (initialize(&g, argv[1], format))
    {
        return 1;
    }

    // Print numbers of nodes and edges
    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    // Set PageRank parameters
    FPTYPE alpha = 0.85;
    FPTYPE tol = 1e-8;
    int maxit = atoi(argv[4]);

    // Initialize PageRank vectors
    FPTYPE *x = malloc(g.n * sizeof(FPTYPE));
    //FPTYPE *y[NTHREADS];
    //for (i = 0; i < NTHREADS; i++)
    //{
    //    y[i] = malloc(g.n * sizeof(FPTYPE));
    //}
    FPTYPE *y[1];
    y[0] = malloc(g.n * sizeof(FPTYPE));

    // Test which algorithm to use
    if (!strcmp(argv[3], "power"))
    {
        // Perform PowerIteration
        power(&g, alpha, tol, maxit, x, y);
    }
    else
    {
        // Invalid algorithm
        fprintf(stderr, "Unknown algorithm.\n");
        free(x);
        //for (i = 0; i < NTHREADS; i++)
        //{
        //    free(y[i]);
        //}
        free(y[0]);
        return 1;
    }

    // Test
    fprintf(stderr, "\n");
    for (i = 0; i < 10; i++)
    {
        fprintf(stderr, "%d: %e\n", i, x[i]);
    }
   
    // Destroy PageRank vectors
    free(x);
    //for (i = 0; i < NTHREADS; i++)
    //{
    //    free(y[i]);
    //}
    free(y[0]);

    return 0;
}

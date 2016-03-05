#include "graph.h"

#define FPTYPE float

/* Floating-point array or file pointer */
union prvector
{
    FPTYPE *array;
    FILE *file;
};

typedef union prvector prvector;

/* Perform one iteration of PowerIteration. */
int poweriterate(graph *g, FPTYPE alpha, prvector x, prvector y, char ooc)
{
    // Initialize y to 0
    FPTYPE init = 0.0;
    unsigned int i;
    for (i = 0; i < g->n; i++)
    {
        if (!ooc)
        {
            y.array[i] = init;
        }
        else
        {
            fwrite(&init, sizeof(FPTYPE), 1, y.file);
        }
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

                // Compute update for neighbors
                if (v.deg != 0)
                {
                    if (!ooc)
                    {
                        FPTYPE update = alpha * x.array[i] / v.deg;

                        unsigned int j;                    
                        for (j = 0; j < v.deg; j++)
                        {
                            #pragma omp atomic
                            y.array[v.adj[j]] += update;
                        }
                    }
                    else
                    {
                        FPTYPE xval;
                        fseeko(x.file, sizeof(FPTYPE) * i, SEEK_SET);
                        fread(&xval, sizeof(FPTYPE), 1, x.file);
                        FPTYPE update = alpha * xval / v.deg;
                        
                        unsigned int j;
                        for (j = 0; j < v.deg; j++)
                        {
                            FPTYPE yval;
                            fseeko(y.file, sizeof(FPTYPE) * v.adj[j], SEEK_SET);
                            fread(&yval, sizeof(FPTYPE), 1, y.file);
                            yval += update;
                            fseeko(y.file, -sizeof(FPTYPE), SEEK_CUR);
                            fwrite(&yval, sizeof(FPTYPE), 1, y.file);
                        }
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

    // Distribute remaining weight among the nodes
    FPTYPE remainder = 1.0;
    if (ooc)
    {
        fseeko(y.file, 0, SEEK_SET);
    }
    for (i = 0; i < g->n; i++)
    {
        if (!ooc)
        {
            remainder -= y.array[i];
        }
        else
        {
            FPTYPE yval;
            fread(&yval, sizeof(FPTYPE), 1, y.file);
            remainder -= yval;
        }
    }
    remainder /= (FPTYPE) g->n;
    if (ooc)
    {
        fseeko(y.file, 0, SEEK_SET);
    }
    for (i = 0; i < g->n; i++)
    {
        if (!ooc)
        {
            y.array[i] += remainder;
        }
        else
        {
            FPTYPE yval;
            fread(&yval, sizeof(FPTYPE), 1, y.file);
            yval += remainder;
            fseeko(y.file, -sizeof(FPTYPE), SEEK_CUR);
            fwrite(&yval, sizeof(FPTYPE), 1, y.file);
        }
    }
    
    return 0;
}

/* Perform PowerIteration. */
int power(graph *g, FPTYPE alpha, FPTYPE tol, int maxit, prvector x, prvector y, char ooc)
{
    // Initialize x to e/n
    FPTYPE init = 1.0 / (FPTYPE) g->n;
    unsigned int i;
    for (i = 0; i < g->n; i++)
    {
        if (!ooc)
        {
            x.array[i] = init;
        }
        else
        {
            fwrite(&init, sizeof(FPTYPE), 1, x.file);
        }
    }

    // Get next blocks
    for (i = 0; i < NTHREADS; i++)
    {
        nextblock(g, i);
    }

    // For each iteration
    int iter = 0;
    while (iter < maxit)
    {
        // Perform iteration
        poweriterate(g, alpha, x, y, ooc);
        iter++;
        
        // Compute residual norm
        FPTYPE norm = 0.0;
        if (ooc)
        {
            fseeko(x.file, 0, SEEK_SET);
            fseeko(y.file, 0, SEEK_SET);
        }
        for (i = 0; i < g->n; i++)
        {
            if (!ooc)
            {
                norm += fabs(x.array[i] - y.array[i]);
            }
            else
            {
                FPTYPE xval;
                FPTYPE yval;
                fread(&xval, sizeof(FPTYPE), 1, x.file);
                fread(&yval, sizeof(FPTYPE), 1, y.file);
                norm += fabs(xval - yval);
            }
        }
        
        // Print residual norm
        fprintf(stderr, "%d: %e\n", iter, norm);
        
        // Copy y to x
        if (!ooc)
        {
            for (i = 0; i < g->n; i++)
            {
                x.array[i] = y.array[i];
            }
        }
        else
        {
            fclose(x.file);
            fclose(y.file);
            rename("xfile.tmp", "zfile.tmp");
            rename("yfile.tmp", "xfile.tmp");
            rename("zfile.tmp", "yfile.tmp");
            x.file = fopen("xfile.tmp", "r+");
            y.file = fopen("yfile.tmp", "r+");
        }

        // Stop iterating if residual norm is within tolerance
        if (norm < tol)
        {
            break;
        }
    }

    return 0;
}

/* Computes the PageRank vector of a graph in
 * BADJ format using PowerIteration. */ 
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./pagerank [BADJ file] [maxiter] [optional out file]\n");
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

    // Set PageRank parameters
    FPTYPE alpha = 0.85;
    FPTYPE tol = 1e-8;
    int maxit = atoi(argv[2]);

    // Determine whether vectors are out of core
    char ooc = 0;
    if (g.n > 67108864)
    {
        ooc = 1;
    }

    // Initialize PageRank vectors
    prvector x;
    prvector y;
    if (!ooc)
    {
        x.array = malloc(g.n * sizeof(FPTYPE));
        y.array = malloc(g.n * sizeof(FPTYPE));
    }
    else
    {
        x.file = fopen("xfile.tmp", "w+");
        y.file = fopen("yfile.tmp", "w+");
    }

    // Perform PowerIteration
    power(&g, alpha, tol, maxit, x, y, ooc);
    
    // Optionally output x and destroy PageRank vectors
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
                fwrite(x.array, sizeof(FPTYPE), g.n, out);
                fclose(out);
            }
        }
        free(x.array);
        free(y.array);
    }
    else
    {
        if (argc > 3)
        {
            rename("xfile.tmp", argv[3]);
            fclose(x.file);
        }
        else
        {
            fclose(x.file);
            remove("xfile.tmp");
        }
        fclose(y.file);
        remove("yfile.tmp");
    }

    // Destroy graph
    destroy(&g);

    return 0;
}

#include "graph.h"

/* Arguments for powercompute */
struct powercomputeargs
{
    graph *g;               // graph
    double alpha;           // alpha
    double *x;              // x
    double *y;              // y
    unsigned int threadno;  // thread number
};

typedef struct powercomputeargs powercomputeargs;

/* Perform PowerIteration computation. */
void *powercompute(void *vpca)
{
    // Get arguments
    powercomputeargs *pca = (powercomputeargs *) vpca;
    graph *g = pca->g;
    double alpha = pca->alpha;
    double *x = pca->x;
    double *y = pca->y;
    unsigned int threadno = pca->threadno;

    // Perform PowerIteration computation
    unsigned int i, j;
    node v;
    double prob;
    while (1)
    {
        // Get the next node
        i = nextnode(g, &v, threadno);
        if (i == (unsigned int) -1)
        {
            break;
        }

        // Compute
        if (v.deg != 0)
        {
            prob = alpha / v.deg;
            for (j = 0; j < v.deg; j++)
            {
                if (g->format == BADJBLK)
                {
                    y[v.adj[j]] += prob * x[i];
                }
            }
        }
    }
    
    return NULL;
}

/* Perform one iteration of PowerIteration. */
int poweriterate(graph *g, double alpha, double *x, double *y[])
{
    // Declare variables
    unsigned int i;
    unsigned int j;
    char firstloop = 1;
    powercomputeargs pca[NTHREADS];

    // Initialize y
    for (i = 0; i < NTHREADS; i++)
    {
        for (j = 0; j < g->n; j++)
        {
            y[i][j] = 0.0;
        }
    }

    while (1)
    {
        // Dispatch computation threads
        for (i = 0; i < NTHREADS; i++)
        {
            pca[i].g = g;
            pca[i].alpha = alpha;
            pca[i].x = x;
            pca[i].y = y[i];
            pca[i].threadno = i;
            pthread_create(&g->comp[i], &g->attr, powercompute, (void *) &pca[i]);
        }

        // Wait for computation threads
        for (i = 0; i < NTHREADS; i++)
        {
            pthread_join(g->comp[i], NULL);
        }
                
        // Wait for reader thread
        pthread_join(g->reader, NULL);

        // Get next blocks
        nextblocks(g);

        // Dispatch reader thread
        pthread_create(&g->reader, &g->attr, loadblocks, (void *) g);

        // Check if iteration is over
        if (!firstloop && g->currblockno[0] == 1)
        {
            break;
        }

        // The next loop is not the first loop
        firstloop = 0;
    }

    // Add y vectors
    for (i = 1; i < NTHREADS; i++)
    {
        for (j = 0; j < g->n; j++)
        {
            y[0][j] += y[i][j];
        }
    }
    
    // Distribute remaining weight among the nodes
    double remainder = 1.0;
    for (i = 0; i < g->n; i++)
    {
        remainder -= y[0][i];
    }
    remainder /= (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        y[0][i] += remainder;
    }

    return 0;
}

/* Perform PowerIteration. */
int power(graph *g, double alpha, double tol, int maxit, double *x, double *y[])
{
    // Dispatch reader thread
    pthread_create(&g->reader, &g->attr, loadblocks, (void *) g);

    // Initialize x to e/n
    unsigned int i;
    double init = 1.0 / (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        x[i] = init;
    }

    // Declare variables
    int iter = 0;
    double norm;

    // Wait for reader thread
    pthread_join(g->reader, NULL);

    // Get next blocks
    nextblocks(g);

    // Dispatch reader thread
    pthread_create(&g->reader, &g->attr, loadblocks, (void *) g);

    // For each iteration
    while (iter < maxit)
    {
        // Perform iteration
        poweriterate(g, alpha, x, y);
        iter++;
        
        // Compute residual norm
        norm = 0.0;
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
        fprintf(stderr, "Usage: ./pagerank [graphfile] [badjblk|badjtblk] power [maxiter]\n");
        return 1;
    }
    
    // Get graph format
    char format;
    if (!strcmp(argv[2], "badjblk"))
    {
        format = BADJBLK;
    }
    else if (!strcmp(argv[2], "badjtblk"))
    {
        format = BADJTBLK;
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
    double alpha = 0.85;
    double tol = 1e-8;
    int maxit = atoi(argv[4]);

    // Initialize PageRank vectors
    double *x = malloc(g.n * sizeof(double));
    double *y[NTHREADS];
    for (i = 0; i < NTHREADS; i++)
    {
        y[i] = malloc(g.n * sizeof(double));
    }

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
        for (i = 0; i < NTHREADS; i++)
        {
            free(y[i]);
        }
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
    for (i = 0; i < NTHREADS; i++)
    {
        free(y[i]);
    }

    return 0;
}

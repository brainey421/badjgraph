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
                y[v.adj[j]] += prob * x[i];
            }
        }
    }

    return NULL;
}

/* Perform one iteration of PowerIteration. */
int poweriterate(graph *g, double alpha, double *x, double *y1, double *y2)
{
    // Declare variables
    unsigned int i;
    char firstloop = 1;
    powercomputeargs pca1;
    powercomputeargs pca2;

    // Initialize y
    for (i = 0; i < g->n; i++)
    {
        y1[i] = 0.0;
        y2[i] = 0.0;
    }

    while (1)
    {
        // Wait for reader thread
        pthread_join(g->reader, NULL);

        // Get next blocks
        nextblocks(g);

        // Dispatch reader thread
        pthread_create(&g->reader, &g->attr, loadblocks, (void *) g);

        // Check if iteration is over
        if (!firstloop && g->currblockno1 == 1)
        {
            break;
        }

        // Dispatch computation thread 1
        pca1.g = g;
        pca1.alpha = alpha;
        pca1.x = x;
        pca1.y = y1;
        pca1.threadno = 1;
        pthread_create(&g->comp1, &g->attr, powercompute, (void *) &pca1);
    
        // Dispatch computation thread 2
        pca2.g = g;
        pca2.alpha = alpha;
        pca2.x = x;
        pca2.y = y2;
        pca2.threadno = 2;
        pthread_create(&g->comp2, &g->attr, powercompute, (void *) &pca2);

        // Wait for computation threads
        pthread_join(g->comp1, NULL);
        pthread_join(g->comp2, NULL);

        // The next loop is not the first loop
        firstloop = 0;
    }

    // Add y2 to y1
    for (i = 0; i < g->n; i++)
    {
        y1[i] += y2[i];
    }
    
    // Distribute remaining weight among the nodes
    double remainder = 1.0;
    for (i = 0; i < g->n; i++)
    {
        remainder -= y1[i];
    }
    remainder /= (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        y1[i] += remainder;
    }

    return 0;
}

/* Perform PowerIteration. */
int power(graph *g, double alpha, double tol, int maxit, double *x, double *y1, double *y2)
{
    // Save a pointer to the original x
    double *xoriginal = x;

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
    double diff;
    double *tmp;

    // For each iteration
    while (iter < maxit)
    {
        // Perform iteration
        poweriterate(g, alpha, x, y1, y2);
        iter++;

        // Swap x and y
        tmp = x;
        x = y1;
        y1 = tmp;

        // Compute residual norm
        norm = 0.0;
        for (i = 0; i < g->n; i++)
        {
            diff = x[i] - y1[i];
            if (diff > 0.0)
            {
                norm += diff;
            }
            else
            {
                norm -= diff;
            }
        }
        
        // Print residual norm
        fprintf(stderr, "%d: %e\n", iter, norm);

        // Stop iterating if residual norm is within tolerance
        if (norm < tol)
        {
            break;
        }
    }

    // Store PageRank vector in the original x
    for (i = 0; i < g->n; i++)
    {
        xoriginal[i] = x[i];
    }

    return 0;
}

int updateiterate(graph *g, double alpha, double *x, double *y1, double *y2)
{
    // TODO
    
    return 0;
}

int update(graph *g, double alpha, double tol, int maxit, double *x, double *y1, double *y2)
{
    // TODO

    return 0;
}

/* Computes the PageRank vector of a directed graph in
 * BADJBLK format using PowerIteration or UpdateIteration. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 5)
    {
        fprintf(stderr, "Usage: ./pagerank [graphfile] [badjblk] [power|update] [maxiter]\n");
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
    double *y1 = malloc(g.n * sizeof(double));
    double *y2 = malloc(g.n * sizeof(double));

    // Test which algorithm to use
    if (!strcmp(argv[3], "power"))
    {
        // Perform PowerIteration
        power(&g, alpha, tol, maxit, x, y1, y2);
    }
    else if (!strcmp(argv[3], "update"))
    {
        // Perform UpdateIteration
        update(&g, alpha, tol, maxit, x, y1, y2);
    }
    else
    {
        // Invalid algorithm
        fprintf(stderr, "Unknown algorithm.\n");
        free(x);
        free(y1);
        free(y2);
        return 1;
    }

    // Test
    fprintf(stderr, "\n");
    int i;
    for (i = 0; i < 10; i++)
    {
        fprintf(stderr, "%d: %e\n", i, x[i]);
    }
   
    // Destroy PageRank vectors
    free(x);
    free(y1);
    free(y2);

    return 0;
}

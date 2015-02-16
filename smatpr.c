#include "smatgraph.h"

int iterate(graph *g, double alpha, double *x, double *y)
{
    node v;
    unsigned long long i;

    for (i = 0; i < g->n; i++)
    {
        y[i] = 0.0;
    }

    for (i = 0; i < g->n; i++)
    {
        nextnode(g, &v, i);
        
        if (v.deg != 0)
        {
            double prob = 1.0 / (double) v.deg;

            unsigned long long j;
            for (j = 0; j < v.deg; j++)
            {
                y[v.adj[j]] += alpha*x[i]*prob;
            }
        }

        free(v.adj);
    }

    double sum = 0.0;
    for (i = 0; i < g->n; i++)
    {
        sum += y[i];
    }
    sum = (1.0 - sum) / (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        y[i] += sum;
    }

    rewindedges(g);

    return 0;
}

int power(graph *g, double alpha, double tol, int maxit, double *x)
{
    unsigned long long i;
    double init = 1.0 / (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        x[i] = init;
    }

    double *y = malloc(g->n * sizeof(double));

    int iter = 0;
    double norm;
    while (iter < maxit)
    {
        iterate(g, alpha, x, y);
        iterate(g, alpha, y, x);
        iter += 2;

        norm = 0.0;
        double diff;
        for (i = 0; i < g->n; i++)
        {
            diff = x[i] - y[i];
            if (diff > 0.0)
            {
                norm += diff;
            }
            else
            {
                norm -= diff;
            }
        }

        fprintf(stderr, "After %d iterations: %f\n", iter, norm);

        if (norm < tol)
        {
            break;
        }
    }

    free(y);

    return 0;
}

int main()
{
    graph g;
    initialize(&g, "/media/drive/graphs/cnr-2000.bsmat", BSMAT);

    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    double alpha = 0.85;
    double tol = 1e-8;
    int maxit = 100;

    double *x = malloc(g.n * sizeof(double));
    
    int i;
    for (i = 0; i < g.n; i++)
    {
        x[i] = 1.0 / (double) g.n;
    }

    power(&g, alpha, tol, maxit, x);
    
    fprintf(stderr, "\n");
    for (i = 0; i < 10; i++)
    {
        fprintf(stderr, "PageRank vector: %f\n", x[i]);
    }

    free(x);

    return 0;
}

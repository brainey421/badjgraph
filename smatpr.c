#include "smatgraph.h"

int iterate(graph *g, double alpha, double *x, double *y)
{
    return 0;
}

int power(graph *g, double alpha, double tol, int maxit, double *x)
{
    return 0;
}

int main()
{
    graph g;
    initialize(&g, "/media/drive/graphs/cnr-2000.smat");

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
    
    return 0;
}

#include "graph.h"

int iterate(graph *g, double alpha, double *x, double *y)
{
    node v;
    unsigned long long i;
    double prob;
    unsigned long long j;

    for (i = 0; i < g->n; i++)
    {
        y[i] = 0.0;
    }

    for (i = 0; i < g->n; i++)
    {
        nextnode(g, &v, i);
        
        if (v.deg != 0)
        {
            prob = 1.0 / (double) v.deg;

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
    double remainder = (1.0 - sum) / (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        y[i] += remainder;
    }

    rewindedges(g);

    return 0;
}

int power(graph *g, double alpha, double tol, int maxit, double *x, double *y)
{
    unsigned long long i;
    double init = 1.0 / (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        x[i] = init;
    }

    int iter = 0;
    double norm;
    while (iter < maxit)
    {
        iterate(g, alpha, x, y);
        iter++;

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
        
        double *temp = x;
        x = y;
        y = temp;

        fprintf(stderr, "%d: %e\n", iter, norm);

        if (norm < tol)
        {
            break;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./pagerank [graphfile] [badj|bsmat|smat]\n");
        return 1;
    }
    
    char format;
    if (!strcmp(argv[2], "bsmat"))
    {
        format = BSMAT;
    }
    else if (!strcmp(argv[2], "smat"))
    {
        format = SMAT;
    }
    else if (!strcmp(argv[2], "badj"))
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

    printf("Nodes: %llu\n", g.n);
    printf("Edges: %llu\n\n", g.m);

    double alpha = 0.85;
    double tol = 1e-8;
    int maxit = 1000;

    double *x = malloc(g.n * sizeof(double));
    double *y = malloc(g.n * sizeof(double));
    
    power(&g, alpha, tol, maxit, x, y);
    
    fprintf(stderr, "\n");
    int i;
    for (i = 0; i < 10; i++)
    {
        fprintf(stderr, "PageRank vector: %e\n", x[i]);
    }

    free(x);
    free(y);

    return 0;
}

#include "graph.h"

int poweriterate(graph *g, double alpha, double *x, double *y)
{
    node v;
    unsigned int i;
    double prob;
    unsigned int j;

    double nolinks = 0.0;

    for (i = 0; i < g->n; i++)
    {
        y[i] = 0.0;
    }

    for (i = 0; i < g->n; i++)
    {
        nextnode(g, &v);
            
        if (v.deg != 0)
        {
            prob = alpha / v.deg;
            for (j = 0; j < v.deg; j++)
            {
                y[v.adj[j]] += prob*x[i];
            }
        }
    }

    double remainder = 1.0;
    for (i = 0; i < g->n; i++)
    {
        remainder -= y[i];
    }
    remainder /= (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        y[i] += remainder;
    }

    return 0;
}

int power(graph *g, double alpha, double tol, int maxit, double *x, double *y)
{
    double *xoriginal = x;
    
    unsigned int i;
    double init = 1.0 / (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        x[i] = init;
    }

    int iter = 0;
    double norm;
    double diff;
    double *tmp;

    while (iter < maxit)
    {
        poweriterate(g, alpha, x, y);
        iter++;

        tmp = x;
        x = y;
        y = tmp;

        norm = 0.0;
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
        
        fprintf(stderr, "%d: %e\n", iter, norm);

        if (norm < tol)
        {
            break;
        }
    }

    for (i = 0; i < g->n; i++)
    {
        xoriginal[i] = x[i];
    }

    return 0;
}

int updateiterate(graph *g, double alpha, double *x, double *y)
{
    double zi;
    node v;
    double prob;
    unsigned int i;
    unsigned int j;

    for (i = 0; i < g->n; i++)
    {
        zi = y[i];
        x[i] += zi;

        nextnode(g, &v);
        
        if (v.deg != 0)
        {
            prob = alpha / (double) v.deg;
            for (j = 0; j < v.deg; j++)
            {
                y[v.adj[j]] += prob*zi;
            }
        }

        y[i] -= zi;
    }

    return 0;
}

int update(graph *g, double alpha, double tol, int maxit, double *x, double *y)
{
    unsigned int i;
    double init = 1.0 / (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        x[i] = init;
    }

    int iter = -1;
    double ynorm;
    double xnorm;
    double error;

    poweriterate(g, alpha, x, y);
    iter++;

    ynorm = 0.0;
    for (i = 0; i < g->n; i++)
    {
        y[i] = y[i] - x[i];
        if (y[i] > 0.0)
        {
            ynorm += y[i];
        }
        else
        {
            ynorm -= y[i];
        }
    }

    fprintf(stderr, "%d: %e\n", iter, ynorm);

    while (iter < maxit)
    {
        updateiterate(g, alpha, x, y);
        iter++;

        ynorm = 0.0;
        for (i = 0; i < g->n; i++)
        {
            if (y[i] > 0.0)
            {
                ynorm += y[i];
            }
            else
            {
                ynorm -= y[i];
            }
        }
       
        xnorm = 0.0;
        for (i = 0; i < g->n; i++)
        {
            if (x[i] > 0.0)
            {
                xnorm += x[i];
            }
            else
            {
                xnorm -= x[i];
            }
        }

        error = ynorm / xnorm;

        fprintf(stderr, "%d: %e\n", iter, error);

        if (error < tol)
        {
            break;
        }
    }

    for (i = 0; i < g->n; i++)
    {
        x[i] = x[i] / xnorm;
    }

    return 0;
}

/* Computes the PageRank vector of a directed graph in
 * BADJ format using PowerIteration or UpdateIteration. */
int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage: ./pagerank [graphfile] [badj|badjblk] [power|update] [maxiter]\n");
        return 1;
    }
    
    char format;
    if (!strcmp(argv[2], "badj"))
    {
        format = BADJ;
    }
    else if (!strcmp(argv[2], "badjblk"))
    {
        format = BADJBLK;
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

    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    double alpha = 0.85;
    double tol = 1e-8;
    int maxit = atoi(argv[4]);

    double *x = malloc(g.n * sizeof(double));
    double *y = malloc(g.n * sizeof(double));
    
    if (!strcmp(argv[3], "power"))
    {
        power(&g, alpha, tol, maxit, x, y);
    }
    else if (!strcmp(argv[3], "update"))
    {
        update(&g, alpha, tol, maxit, x, y);
    }
    else
    {
        fprintf(stderr, "Unknown algorithm.\n");
        free(x);
        free(y);
        return 1;
    }

    // Test
    fprintf(stderr, "\n");
    int i;
    for (i = 0; i < 10; i++)
    {
        fprintf(stderr, "%d: %e\n", i, x[i]);
    }
   
    free(x);
    free(y);

    return 0;
}

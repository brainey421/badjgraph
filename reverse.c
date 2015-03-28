#include "graph.h"

/* Reverses the order of the edges in a graph in BADJ format. */
/* OUTDATED -- should use unsigned int, not unsigned long long. */
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./reverse [graphfile] [newfile]\n");
        return 1;
    }
    
    graph g;
    if (initialize(&g, argv[1], BADJ))
    {
        return 1;
    }

    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n", g.m);

    FILE *out = fopen(argv[2], "w");
    fwrite(&g.n, sizeof(unsigned long long), 1, out);
    fwrite(&g.m, sizeof(unsigned long long), 1, out);

    fprintf(stderr, "Recording degree of each node ...\n");

    unsigned long long *degree = malloc(g.n*sizeof(unsigned long long));
    long long i, j;
    for (i = 0; i < g.n; i++)
    {
        fread(&degree[i], sizeof(unsigned long long), 1, g.stream);

        fseek(g.stream, degree[i]*sizeof(unsigned long long), SEEK_CUR);
    }

    fprintf(stderr, "Reversing order of edges ...\n");
    unsigned long long *node;
    for (i = g.n - 1; i >= 0; i--)
    {
        if (i < g.n - 1)
        {
            fseek(g.stream, (-degree[i+1] - 1)*sizeof(unsigned long long), SEEK_CUR);
        }
        fseek(g.stream, (-degree[i] - 1)*sizeof(unsigned long long), SEEK_CUR);

        node = malloc((degree[i] + 1)*sizeof(unsigned long long));
        fread(node, sizeof(unsigned long long), degree[i] + 1, g.stream);
        for (j = 1; j < degree[i] + 1; j++)
        {
            node[j] = g.n - node[j] - 1;
        }
        fwrite(node, sizeof(unsigned long long), degree[i] + 1, out);
        free(node);
    }

    free(degree);

    return 0;
}

#include "graph.h"

/* Sorts the nodes in a graph in BADJ format by degree in descending order. */
/* OUTDATED -- should use unsigned int, not unsigned long long. */
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./sortdegree [graphfile] [newfile]\n");
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

    fprintf(stderr, "Recording degree and offset of each node ...\n");

    unsigned long long *degree = malloc(g.n*sizeof(unsigned long long));
    fpos_t *offset = malloc(g.n*sizeof(fpos_t));
    long long i, j;
    for (i = 0; i < g.n; i++)
    {
        fgetpos(g.stream, &offset[i]);
        fread(&degree[i], sizeof(unsigned long long), 1, g.stream);

        fseek(g.stream, degree[i]*sizeof(unsigned long long), SEEK_CUR);
    }

    fprintf(stderr, "Sorting nodes in descending order of degree ...\n");
    
    unsigned long long *node = malloc(g.n*sizeof(unsigned long long));
    for (i = 0; i < g.n; i++)
    {
        node[i] = i;
    }

    unsigned long long tmp;
    for (i = g.n - 1; i >= 0; i--)
    {
        for (j = 0; j < i; j++)
        {
            if (degree[node[j]] < degree[node[j+1]])
            {
                tmp = node[j];
                node[j] = node[j+1];
                node[j+1] = tmp;
            }
        }
    }
    
    unsigned long long *newpos = malloc(g.n*sizeof(unsigned long long));
    for (i = 0; i < g.n; i++)
    {
        newpos[node[i]] = i;
    }

    fprintf(stderr, "Writing sorted nodes ...\n");

    unsigned long long *mynode;
    for (i = 0; i < g.n; i++)
    {
        fsetpos(g.stream, &offset[node[i]]);

        mynode = malloc((degree[node[i]] + 1)*sizeof(unsigned long long));
        fread(mynode, sizeof(unsigned long long), degree[node[i]] + 1, g.stream);
        for (j = 1; j < degree[node[i]] + 1; j++)
        {
            mynode[j] = newpos[mynode[j]];
        }
        fwrite(mynode, sizeof(unsigned long long), degree[node[i]] + 1, out);
        free(mynode);
    }

    free(degree);
    free(offset);
    free(newpos);
    free(node);
        
    return 0;
}

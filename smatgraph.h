#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAMELEN 1024
#define BUFFSIZE    32

/*
 * Graph in SMAT format
 */
struct graph
{
    char filename[FILENAMELEN];
    FILE *stream;

    unsigned long long n;       // number of nodes
    unsigned long long m;       // number of edges
};

/*
 * Edge
 */
struct edge
{
    unsigned long long src;     // source node
    unsigned long long dest;    // destination node
    int weight;                 // edge weight
};

typedef struct graph graph;
typedef struct edge edge;

int initialize(graph *g, char *filename);   // initialize graph
int nextedge(graph *g, edge *e);                   // get next edge

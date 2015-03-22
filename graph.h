#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zlib.h"

#define FILENAMELEN 1024
#define BUFFSIZE    32

#define SMAT        0
#define BSMAT       1
#define BADJ        2
#define BADJGZ      3

/*
 * Graph in SMAT/BSMAT/BADJ/BADJGZ format
 */
struct graph
{
    char filename[FILENAMELEN]; // name of graph file
    FILE *stream;               // pointer to graph file
    gzFile gzstream;            // compressed graph file
    char format;                // format of graph file

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
};

/*
 * Node
 */
struct node
{
    unsigned long long deg;     // out-degree
    unsigned long long *adj;    // adjacent nodes
};

typedef struct graph graph;
typedef struct edge edge;
typedef struct node node;

edge recentedge;                // most recently read edge

int initialize(graph *g, char *filename, char format);  // initialize graph
int nextedge(graph *g, edge *e);                        // get next edge
int nextnode(graph *g, node *v, unsigned long long i);  // get next node; must call free(v->adj) after
int rewindedges(graph *g);                              // rewind edge file pointer

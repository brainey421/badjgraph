#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAMELEN 1024
#define BLOCKLEN    16777216
#define MAXBLKS     32768
#define MAXNODES    4294967296
#define NTHREADS    8

/* Graph in BADJ format */
struct graph
{
    char filename[FILENAMELEN];             // name of graph file
    FILE *stream;                           // pointer to graph file
    char badji;                             // whether graph has a badji file

    unsigned long long n;                   // number of nodes
    unsigned long long m;                   // number of edges
    unsigned long long nblks;               // number of blocks
    unsigned long long *indices;            // indices of blocks in graph file
    unsigned int *firstnodes;               // first nodes in blocks

    FILE *currblock[NTHREADS];              // current block file pointers
    unsigned int currblockno[NTHREADS];     // current block numbers
    unsigned int currnode[NTHREADS];        // current nodes
};

/* Node */
struct node
{
    unsigned int deg;       // out-degree
    unsigned int *adj;      // adjacent nodes
};

typedef struct graph graph;
typedef struct node node;

int initialize(graph *g, char *filename, char badji);               // initialize graph
int destroy(graph *g);                                              // destroy graph
int transpose(graph *g, char *filename);                            // transpose graph
int locality(graph *g, unsigned int window, double *locality);      // compute the locality of a graph
int badjindex(graph *g);                                            // create a badji file for a BADJ graph
int nextblock(graph *g, unsigned int threadno);                     // get the next block of the graph
unsigned int nextnode(graph *g, node *v, unsigned int threadno);    // get the next node of the block

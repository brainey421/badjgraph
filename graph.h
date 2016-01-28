#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAMELEN 1024
#define BLOCKLEN    16777216
#define MAXBLKS     1024
#define MAXTOPLEN   128
#define TOPLEN      128
#define NTHREADS    8

#define BADJ        0
#define BADJBLK     1

/* Graph in BADJ/BADJBLK format */
struct graph
{
    char filename[FILENAMELEN];             // name of graph file
    FILE *stream;                           // pointer to graph file
    char format;                            // format of graph

    FILE *currblock[NTHREADS];              // current block file pointer
    unsigned int currblockno[NTHREADS];     // current block numbers
    unsigned int currnode[NTHREADS];        // current nodes

    unsigned long long n;                   // number of nodes
    unsigned long long m;                   // number of edges
    unsigned long long nblks;               // number of blocks
    unsigned long long *indices;            // indices of blocks
    unsigned int *firstnodes;               // first nodes in blocks
    unsigned int *topnodes;                 // degrees of nodes
};

/* Node */
struct node
{
    unsigned int deg;       // out-degree
    unsigned int *adj;      // adjacent nodes
};

typedef struct graph graph;
typedef struct node node;

int initialize(graph *g, char *filename, char format);              // initialize graph
int partition(graph *g);                                            // partition a BADJ graph into a BADJBLK graph
int transpose(graph *g, char *dirname);                             // transpose a BADJ graph
int nextblock(graph *g, unsigned int threadno);                     // get the next block of the graph
unsigned int nextnode(graph *g, node *v, unsigned int threadno);    // get next node in the graph

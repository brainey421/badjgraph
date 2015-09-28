#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAMELEN 1024
#define BLOCKLEN    16777216
#define MAXBLKS     1024
#define NTHREADS    3

#define BADJ        0
#define BADJBLK     1

/* Graph in BADJ/BADJBLK format */
struct graph
{
    char filename[FILENAMELEN];             // name of graph file
    FILE *stream;                           // pointer to graph file
    char format;                            // format of graph

    unsigned int *currblock[NTHREADS];      // current blocks
    unsigned int currblocklen[NTHREADS];    // current block lengths
    unsigned int currblockno[NTHREADS];     // current block numbers
    unsigned int currblocki[NTHREADS];      // current block indices
    unsigned int currnode[NTHREADS];        // current nodes

    unsigned int *nextblock[NTHREADS];      // next blocks
    unsigned int nextblocklen[NTHREADS];    // next block lengths
    unsigned int nextblockno[NTHREADS];     // next block numbers

    pthread_t reader;                       // reader thread
    pthread_t comp[NTHREADS];               // computation threads
    pthread_attr_t attr;                    // thread attributes

    unsigned long long n;                   // number of nodes
    unsigned long long m;                   // number of edges
    unsigned long long nblks;               // number of blocks
    unsigned int *firstnodes;               // first nodes in blocks
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
int partition(graph *g, char *dirname);                             // partition a BADJ graph into a BADJBLK graph
void *loadblocks(void *vg);                                         // read the next blocks in the graph
int nextblocks(graph *g);                                           // get the next blocks of the graph
unsigned int nextnode(graph *g, node *v, unsigned int threadno);    // get next node in the graph

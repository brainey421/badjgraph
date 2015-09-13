#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAMELEN 1024
#define BLOCKLEN    16777216
#define MAXBLKS     1024

#define BADJ        0
#define BADJBLK     1

/* Graph in BADJ/BADJBLK format */
struct graph
{
    char filename[FILENAMELEN];         // name of graph file
    FILE *stream;                       // pointer to graph file
    char format;                        // format of graph

    unsigned int *currblock1;           // current block 1
    unsigned int currblocklen1;         // length of current block 1
    unsigned int currblockno1;          // current block number 1
    unsigned int currblocki1;           // index within current block 1
    unsigned int currnode1;             // current node 1

    unsigned int *currblock2;           // current block 2
    unsigned int currblocklen2;         // length of current block 2
    unsigned int currblockno2;          // current block number 2
    unsigned int currblocki2;           // index within current block 2
    unsigned int currnode2;             // current node 2

    unsigned int *nextblock1;           // next block 1
    unsigned int nextblocklen1;         // length of next block 1
    unsigned int nextblockno1;          // next block number 1
    unsigned int nextnode1;             // next node 1

    unsigned int *nextblock2;           // next block 2
    unsigned int nextblocklen2;         // length of next block 2
    unsigned int nextblockno2;          // next block number 2
    unsigned int nextnode2;             // next node 2

    pthread_t reader;                   // reader thread
    pthread_t comp1;                    // computation thread 1
    pthread_t comp2;                    // computation thread 2
    pthread_attr_t attr;                // thread attributes
    pthread_mutex_t lock;               // mutex lock

    unsigned long long n;               // number of nodes
    unsigned long long m;               // number of edges
    unsigned long long nblks;           // number of blocks
    unsigned int *firstnodes;           // first nodes in blocks
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
void *loadblocks(void *vg);                                         // read the next two blocks in the graph
int nextblocks(graph *g);                                           // get the next two blocks of the graph
unsigned int nextnode(graph *g, node *v, unsigned int threadno);    // get next node in the graph

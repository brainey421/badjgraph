#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAMELEN 1024
#define BLOCKLEN    67108864
        
#define BADJ        0
#define BADJBLK     1

/*
 * Graph in BADJ/BADJBLK format
 */
struct graph
{
    char filename[FILENAMELEN];         // name of graph file
    FILE *stream;                       // pointer to graph file
    char format;                        // format of graph

    unsigned int *currblock;            // current block
    unsigned int currblocklen;          // length of current block
    unsigned int currblocki;            // index within current block

    unsigned int *nextblock;            // next block
    unsigned int nextblocklen;          // length of next block
    unsigned int nextblockno;           // next block number

    pthread_t reader;                   // reader thread
    pthread_attr_t attr;                // reader thread attributes

    unsigned long long n;               // number of nodes
    unsigned long long m;               // number of edges
    unsigned long long nblks;           // number of blocks
};

/*
 * Node
 */
struct node
{
    unsigned int deg;       // out-degree
    unsigned int *adj;      // adjacent nodes
};

typedef struct graph graph;
typedef struct node node;

int initialize(graph *g, char *filename, char format);  // initialize graph
int partition(graph *g, char *dirname);                 // partition graph into blocks
void *loadblock(void *vg);                              // load next block
int nextnode(graph *g, node *v);                        // get next node

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAMELEN 1024
#define BUFFSIZE    32

/*
 * Graph in SMAT format.
 */
struct graph
{
    char filename[FILENAMELEN];
    FILE *stream;

    unsigned long long n;   // number of nodes
    unsigned long long m;   // number of edges
};

typedef struct graph graph;

int initialize(graph *g, char *filename);   // initialize graph

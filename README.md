# BADJ PageRank

A C implementation of the power iteration algorithm to compute the PageRank vector for graphs in BADJ format.

## Example Usage

		$ make
		$ ./pagerank data/wb-cs.stanford.badj badj power 1000

## BADJ Format

BADJ stands for "binary adjacency list." 
A graph in BADJ format looks like this:

- Number of nodes (8-byte integer)
- Number of edges (8-byte integer)
- A list of nodes in the following format: out-degree, list of adjacent nodes (all 4-byte integers)

There are sample BADJ files in the data directory.

## Multithreading and Blocks

This PageRank implementation uses two concurrent threads: one to read a block of the graph into memory, and another to perform computation on that block. 
By default, each block is at most 64MB, as defined in graph.h by BLOCKLEN, and a node's adjacency list does not straddle two blocks.

This repository includes a program to partition a graph in BADJ format into its blocks explicitly. 
The resulting graph in BADJBLK format is a directory containing one file for each block. 
In the future, the PageRank program will support compressed blocks to reduce time spent reading the graph. 

		$ ./partition
		Usage: ./partition [graphfile] [badj] [outdirectory]

## Reading a BADJ File

Here is an example of how to read through a graph in BADJ format:

		graph g;
		initialize(&g, "data/wb-cs.stanford.badj", BADJ);
		unsigned int i, j;
		node v;
		for (i = 0; i < g.n; i++) {
			nextnode(&g, &v, i);
			printf("Degree: %d, ", v.deg);
			printf("Adjacent nodes:");
			for (j = 0; j < v.deg; j++) {
				printf(" %d", v.adj[j]);
			}
			printf("\n");
		}

This repository already includes a program to read through a graph.

		$ ./readedges 
		Usage: ./readedges [graphfile] [badj|badjblk] [niterations]

## Computing PageRank

By default, this power iteration implementation uses alpha = 0.85 and iterates until achieving a residual norm of 1e-8.

		 $ ./pagerank
		Usage: ./pagerank [graphfile] [badj|badjblk] [power|update] [maxiter]

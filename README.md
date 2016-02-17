# BADJGraph

BADJGraph is a C implementation of computations on graphs in BADJ (binary adjacency list) format. 
It includes tools to stream graphs from disk, 
perform the power iteration algorithm to compute PageRank, 
and perform the label propagation algorithm to identify connected components.

## Example Usage

        $ make
        $ ./partition data/wb-cs.stanford.badj
        $ ./pagerank data/wb-cs.stanford.badjblk 20

## BADJ Format

BADJ stands for "binary adjacency list." 
A graph in BADJ format looks like this:

- Number of nodes (8-byte integer)
- Number of edges (8-byte integer)
- A list of nodes in the following format: out-degree, list of adjacent nodes (all 4-byte integers)

There are sample BADJ files in the data directory.

## BADJBLK Format

BADJBLK stands for "BADJ block." 
A graph in BADJBLK format includes metadeta that effectively splits a BADJ graph into blocks of size BLOCKLEN (graph.h).
Partitioning a BADJ graph into a BADJBLK graph allows multithreaded computations that divide the blocks among NTHREADS (graph.h) threads. 

        $ ./partition
        Usage: ./partition [BADJ graph]

## Streaming BADJBLK Graphs

        $ ./stream
        Usage: ./stream [BADJBLK graph]

## Computing PageRank

By default, this power iteration implementation of PageRank uses alpha = 0.85 and iterates until achieving a residual norm of 1e-8 (pagerank.c). 

		 $ ./pagerank
		Usage: ./pagerank [BADJBLK graph] [maxiter]

## Computing Connected Components

        $ ./components
        Usage: ./components [BADJBLK graph] [maxiter]

# BADJ PageRank

A C implementation of the power iteration algorithm to compute the PageRank vector for graphs in BADJ format.

## BADJ Format

BADJ stands for "binary adjacency list." 
A graph in BADJ format looks like this:

- Number of nodes (8-byte integer)
- Number of edges (8-byte integer)
- A list of nodes in the following format: out-degree, list of adjacent nodes (all 4-byte integers)

There are sample BADJ files in the data directory.

## Example Usage

		make
		./pagerank data/wb-cs.stanford.badj badj power 1000

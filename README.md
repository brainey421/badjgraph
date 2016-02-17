# BADJGraph

A C implementation of computations on graphs in BADJ (binary adjacency list) format. 
Includes tools to stream graphs from disk, 
perform the power iteration algorithm to compute PageRank, 
and the label propagation algorithm to identify connected components.

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
BADJBLK format is similar to BADJ format, but it includes metadeta that effectively splits the graph into blocks. 
By default, the size of each block is 16MB, but this can be modified with the BLOCKLEN macro in "graph.h."
The "partition" program converts a BADJ graph to BADJBLK format:

        $ ./partition
        Usage: ./partition [BADJ graph]

Partitioning a BADJ graph into BADJBLK format allows parallelized computation in which each thread performs computation on certain blocks in the graph. 
By default, the number of threads is 8, but this can be modified with the NTHREADS macro in "graph.h."

## Streaming BADJBLK Graphs

Here is an example of how to stream a BADJBLK graph from disk when NTHREADS is 1:

		graph g;
		initialize(&g, "data/wb-cs.stanford.badj", BADJBLK);
        nextblock(&g, 0);
		while (1)
        {
            node v;
			unsigned int i = nextnode(&g, &v);
            if (i == (unsigned int) -1)
            {
                nextblock(&g, 0);
                if (g->currblockno[0] <= NTHREADS)
                {
                    break;
                }
            }

			printf("Nodes adjacent to node %d:", i);
			for (j = 0; j < v.deg; j++)
            {
				printf(" %d", v.adj[j]);
			}
			printf("\n");
		}
        destroy(&g);

## Computing PageRank

By default, this power iteration implementation uses alpha = 0.85 and iterates until achieving a residual norm of 1e-8. 
These parameters can be changed in "pagerank.c."

		 $ ./pagerank
		Usage: ./pagerank [BADJBLK graph] [maxiter]

## Computing Connected Components

        $ ./components
        Usage: ./components [BADJBLK graph] [maxiter]

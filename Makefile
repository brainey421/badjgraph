# Assumes that zlib is installed and configured.

all: pagerank readedges partition

pagerank: pagerank.c graph.o
	gcc -o pagerank pagerank.c graph.o -pthread

readedges: readedges.c graph.o
	gcc -o readedges readedges.c graph.o -pthread

partition: partition.c graph.o
	gcc -o partition partition.c graph.o -pthread

graph.o: graph.c graph.h
	gcc -c graph.c -pthread

clean:
	rm -f graph.o
	rm -f pagerank
	rm -f readedges
	rm -f partition

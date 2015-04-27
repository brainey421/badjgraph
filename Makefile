# Assumes that zlib is installed and configured.

all: pagerank readedges

pagerank: pagerank.c graph.o
	gcc -o pagerank pagerank.c graph.o

readedges: readedges.c graph.o
	gcc -o readedges readedges.c graph.o

graph.o: graph.c graph.h
	gcc -c graph.c

clean:
	rm -f graph.o
	rm -f pagerank
	rm -f readedges

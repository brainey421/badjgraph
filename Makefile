LDFLAGS += -fopenmp
CFLAGS += -O3 -Wall -D_LARGEFILE64_SOURCE

all: pagerank readedges partition transpose

pagerank: pagerank.c graph.o

readedges: readedges.c graph.o

partition: partition.c graph.o

transpose: transpose.c graph.o

graph.o: graph.c graph.h

clean:
	rm -f graph.o
	rm -f pagerank
	rm -f readedges
	rm -f transpose
	rm -f partition

LDFLAGS += -fopenmp
CFLAGS += -O3 -Wall -D_LARGEFILE64_SOURCE

all: pagerank components partition transpose

pagerank: pagerank.c graph.o

components: components.c graph.o

partition: partition.c graph.o

transpose: transpose.c graph.o

graph.o: graph.c graph.h

clean:
	rm -f graph.o
	rm -f pagerank
	rm -f components
	rm -f transpose
	rm -f partition

LDFLAGS += -fopenmp
CFLAGS += -O3 -Wall -Wno-unused-result -D_FILE_OFFSET_BITS="64" -D_LARGEFILE64_SOURCE

all: partition stream pagerank components

partition: partition.c graph.o

stream: stream.c graph.o

pagerank: pagerank.c graph.o

components: components.c graph.o

graph.o: graph.c graph.h


clean:
	rm -f graph.o
	rm -f partition
	rm -f stream
	rm -f pagerank
	rm -f components

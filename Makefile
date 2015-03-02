all: pagerank

pagerank: pagerank.c graph.o
	gcc -o pagerank pagerank.c graph.o

graph.o: graph.c graph.h
	gcc -c graph.c

clean:
	rm -f graph.o
	rm -f pagerank

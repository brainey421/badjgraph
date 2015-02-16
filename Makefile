all: pagerank test

pagerank: pagerank.c graph.o
	gcc -o pagerank pagerank.c graph.o

test: test.c graph.o
	gcc -o test test.c graph.o

graph.o: graph.c
	gcc -c graph.c

clean:
	rm -f graph.o
	rm -f test
	rm -f pagerank

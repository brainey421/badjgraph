all: pagerank reverse sortdegree

pagerank: pagerank.c graph.o
	gcc -o pagerank pagerank.c graph.o

reverse: reverse.c graph.o
	gcc -o reverse reverse.c graph.o

sortdegree: sortdegree.c graph.o
	gcc -o sortdegree sortdegree.c graph.o

graph.o: graph.c graph.h
	gcc -c graph.c

clean:
	rm -f graph.o
	rm -f pagerank
	rm -f reverse
	rm -f sortdegree

all: pagerank reverse sortdegree readedges

pagerank: pagerank.c graph.o
	gcc -o pagerank pagerank.c graph.o -lz

reverse: reverse.c graph.o
	gcc -o reverse reverse.c graph.o -lz

sortdegree: sortdegree.c graph.o
	gcc -o sortdegree sortdegree.c graph.o -lz

readedges: readedges.c graph.o
	gcc -o readedges readedges.c graph.o -lz

graph.o: graph.c graph.h
	gcc -c graph.c -lz

clean:
	rm -f graph.o
	rm -f pagerank
	rm -f reverse
	rm -f sortdegree
	rm -f readedges

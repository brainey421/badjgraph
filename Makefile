all:
	gcc -c smatgraph.c
	gcc -o test test.c smatgraph.o
	gcc -o smatpr smatpr.c smatgraph.o

clean:
	rm -f smatgraph.o
	rm -f test
	rm -f smatpr

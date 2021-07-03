mem-copy-test: mem-copy-test.o mem-copy.o
	g++ -O3 mem-copy-test.o mem-copy.o -o mem-copy-test

mem-copy.o: mem-copy.c mem-copy.h types.h Makefile
	# see source for discussion of why -O1
	gcc -S -O1 mem-copy.c
	gcc -c -O1 mem-copy.c

mem-copy-test.o: mem-copy-test.cpp mem-copy.h types.h
	g++ -c -O3 mem-copy-test.cpp

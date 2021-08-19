lz4-parse: lz4-parse.o decode.o util.o
	g++ -O3 lz4-parse.o decode.o util.o -o lz4-parse

util.o: ../include/util.h ../util/util.cpp
	g++ -c -O3 -Wall -I../include/ ../util/util.cpp

lz4-parse.o: lz4-parse.cpp ../include/decode.h Makefile
	g++ -c -O3 -Wall -I../include/ lz4-parse.cpp

decode.o: ../decode/decode.c ../include/decode.h ../include/decode.h Makefile
	gcc -c -O3 -Wall -I../include/ ../decode/decode.c

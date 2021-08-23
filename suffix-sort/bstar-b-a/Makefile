bstar-b-a: Makefile bstar-b-a.cpp ../../include/types.h ../../include/util.h ../../util/util.cpp Makefile
	g++ -S -fverbose-asm -g -O1 -I../../include/ bstar-b-a.cpp
	as -alhnd bstar-b-a.s > bstar-b-a.lst
	g++ -DBSTAR_B_A_SUFFIX_SORT_MAIN -I ../../include/ -O1 -Wall bstar-b-a.cpp ../../util/util.cpp -o bstar-b-a

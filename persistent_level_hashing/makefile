all: plevel

plevel: test.o level_hashing.o hash.o pflush.o log.o
	gcc -o plevel test.o level_hashing.o hash.o pflush.o log.o libnvmemul.so -lm

hash.o: hash.c hash.h
	gcc -c hash.c
	
level_hashing.o: level_hashing.c level_hashing.h
	gcc -c level_hashing.c

test.o: test.c level_hashing.h
	gcc -c test.c

pflush.o: pflush.c pflush.h
	gcc -c pflush.c

log.o: log.c log.h
	gcc -c log.c

clean:
	rm -rf *.o plevel

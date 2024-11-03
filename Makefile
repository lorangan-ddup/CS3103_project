CC=gcc
CFLAGS=-g -pthread

problem1 : problem1.o utils.o
	$(CC) $(CFLAGS) -o problem1 problem1.o utils.o -lm  

problem1.o : problem1.c utils.h
	$(CC) $(CFLAGS) -c problem1.c

utils.o : utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

.PHONY : clean
clean : 
	rm -f *.o problem1

CC = mpicc
CFLAGS = -Wall -O3 -g
BINS = quicksort

all: $(BINS)

quicksort: quicksort.h quicksort.c pivot.h
	$(CC) $(CFLAGS) -o $@ quicksort.c

clean:
	$(RM) $(BINS)


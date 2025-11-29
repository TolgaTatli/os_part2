CC = g++
CFLAGS = -Wall -std=c++11
PTHREAD_FLAGS = -lpthread
OPENMP_FLAGS = -fopenmp

all: pthreads_pc openmp_pc

pthreads_pc: producer_consumer_pthreads.cpp
	$(CC) $(CFLAGS) -o pthreads_pc producer_consumer_pthreads.cpp $(PTHREAD_FLAGS)

openmp_pc: producer_consumer_openmp.cpp
	$(CC) $(CFLAGS) -o openmp_pc producer_consumer_openmp.cpp $(OPENMP_FLAGS)

clean:
	rm -f pthreads_pc openmp_pc

run_pthreads:
	./pthreads_pc

run_openmp:
	./openmp_pc

.PHONY: all clean run_pthreads run_openmp
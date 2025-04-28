MPICC = mpicc
CFLAGS = -O3 -fopenmp -Wall -Wextra

LDFLAGS = -fopenmp -lm

SRC = mpi.c

EXEC = log_counter

all: $(EXEC)

$(EXEC): $(SRC)
	$(MPICC) $(CFLAGS) $(SRC) -o $(EXEC) $(LDFLAGS)

run: $(EXEC)
	export OMP_NUM_THREADS=2; mpirun -np 4 ./$(EXEC)

clean:
	rm -f $(EXEC) event_count.txt top_10.txt core.* *.o

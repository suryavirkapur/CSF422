#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 10
#define P_MAX 10

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, P;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &P);

    if (P > P_MAX) {
        if (rank == 0)
            printf("Error: too many processes (%d) for max %d\n", P, P_MAX);
        MPI_Finalize();
        return 1;
    }

    int base = N / P, remainder = N % P;
    int rows = base + (rank < remainder ? 1 : 0);

    int local_matrix[N][N], local_avgi[N], global_avgi[N];
    int rows_per_process[P_MAX], displs[P_MAX], sendcounts[P_MAX];
    int matrix[N][N];

    if (rank == 0) {
        srand(time(NULL));
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                matrix[i][j] = rand() % 10;

        for (int i = 0; i < P; i++) {
            int rows_i = base + (i < remainder ? 1 : 0);
            sendcounts[i] = rows_i * N;
            displs[i] = i == 0 ? 0 : displs[i - 1] + sendcounts[i - 1];
        }
    }

    MPI_Scatterv(matrix, sendcounts, displs, MPI_INT, local_matrix, rows * N, MPI_INT, 0, MPI_COMM_WORLD);

    for (int i = 0; i < rows; i++) {
        int sum = 0;
        for (int j = 0; j < N; j++)
            sum += local_matrix[i][j];
        local_avgi[i] = (sum + N - 1) / N;
    }

    for (int i = 0; i < P; i++)
        rows_per_process[i] = base + (i < remainder ? 1 : 0);

    displs[0] = 0;
    for (int i = 1; i < P; i++)
        displs[i] = displs[i - 1] + rows_per_process[i - 1];

    MPI_Allgatherv(local_avgi, rows, MPI_INT, global_avgi, rows_per_process, displs, MPI_INT, MPI_COMM_WORLD);

    int histogram[10] = {0};
    for (int k = 0; k < N; k++)
        histogram[global_avgi[k]]++;

    char filename[20];
    sprintf(filename, "hit_%d.txt", rank);
    FILE *fp = fopen(filename, "w");
    for (int i = 0; i < 10; i++)
        fprintf(fp, "%d: %d\n", i, histogram[i]);
    fclose(fp);

    MPI_Finalize();
    return 0;
}
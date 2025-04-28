/* File:
 *     pth_mat_vect_rand_split.c
 *
 * Purpose:
 *     Computes a parallel matrix-vector product.  Matrix
 *     is distributed by block rows.  Vectors are distributed by
 *     blocks.  This version uses a random number generator to
 *     generate A and x.  It also makes some small changes to
 *     the multiplication.  These are intended to improve
 *     performance and explicitly use temporary variables.
 *
 * Input:
 *     none
 *
 * Output:
 *     y: the product vector
 *     Elapsed time for the computation
 *
 * Compile:
 *    gcc -g -Wall -o pth_mat_vect_rand pth_mat_vect_rand.c -lpthread
 * Usage:
 *     pth_mat_vect <thread_count> <m> <n>
 *
 * Notes:
 *     1.  Local storage for A, x, y is dynamically allocated.
 *     2.  Number of threads (thread_count) should evenly divide
 *         m.  The program doesn't check for this.
 *     3.  We use a 1-dimensional array for A and compute subscripts
 *         using the formula A[i][j] = A[i*n + j]
 *     4.  Distribution of A, x, and y is logical:  all three are
 *         globally shared.
 *     5.  Compile with -DDEBUG for information on generated data
 *         and product.
 */

#include "pthread_barrier.h"
#include "timer.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/* Global variables */
int thread_count;
int m, n;
double *A;
double *x;
double *y;

#define CACHE_LINE 64
struct aligned_double
{
  double value;
  char padding[CACHE_LINE - sizeof(double)];
};
struct aligned_double *y_aligned;

pthread_barrier_t barrier;

/* Serial functions */
void Usage(char *prog_name);
void Gen_matrix(double A[], int m, int n);
void Gen_vector(double x[], int n);
void Print_vector(char *title, double y[], double m);

/* Parallel functions */
void *Pth_mat_vect(void *rank);
void *Pth_mat_vect_no_false_sharing(void *rank);
void *Pth_mat_vect_with_barrier(void *rank);

void print_gflops(double time_taken, int m, int n)
{
  double flops = 2.0 * m * n;
  double gflops = (flops / time_taken) / 1e9;
  printf("gflops/sec: %f", gflops);
}

/*------------------------------------------------------------------*/
int main()
{
  int mxn[3][2] = {{8000000, 8}, {8000, 8000}, {8, 8000000}};

  int num_threads[] = {1, 2, 4, 8};

  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      printf("\ndimensions: %dx%d\t threads: %d\t", mxn[i][0], mxn[i][1], num_threads[j]);

      thread_count = num_threads[j];
      m = mxn[i][0];
      n = mxn[i][1];

      pthread_t *thread_handles = malloc(thread_count * sizeof(pthread_t));
      A = malloc(m * n * sizeof(double));
      x = malloc(n * sizeof(double));
      y = malloc(m * sizeof(double));
      y_aligned = malloc(m * sizeof(struct aligned_double));

      pthread_barrier_init(&barrier, NULL, thread_count);

      Gen_matrix(A, m, n);
      Gen_vector(x, n);

      double start, finish;
      GET_TIME(start);

      for (long thread = 0; thread < thread_count; thread++)
        pthread_create(&thread_handles[thread], NULL, Pth_mat_vect, (void *)thread);

      for (long thread = 0; thread < thread_count; thread++)
        pthread_join(thread_handles[thread], NULL);

      GET_TIME(finish);
      print_gflops(finish - start, m, n);

      free(A);
      free(x);
      free(y);
      free(y_aligned);
      free(thread_handles);
      pthread_barrier_destroy(&barrier);
    }
  }

  return 0;
}

/*------------------------------------------------------------------
 * Function:       Pth_mat_vect
 * Purpose:        Multiply an mxn matrix by an nx1 column vector
 * In arg:         rank
 * Global in vars: A, x, m, n, thread_count
 * Global out var: y
 */
void *Pth_mat_vect(void *rank)
{
  long my_rank = (long)rank;
  int local_m = m / thread_count;
  int my_first_row = my_rank * local_m;
  int my_last_row = my_first_row + local_m;

  for (int i = my_first_row; i < my_last_row; i++)
  {
    double temp = 0.0;
    for (int j = 0; j < n; j++)
      temp += A[i * n + j] * x[j];
    y[i] = temp;
  }

  return NULL;
}

void *Pth_mat_vect_no_false_sharing(void *rank)
{
  long my_rank = (long)rank;
  int local_m = m / thread_count;
  int my_first_row = my_rank * local_m;
  int my_last_row = my_first_row + local_m;

  for (int i = my_first_row; i < my_last_row; i++)
  {
    double temp = 0.0;
    for (int j = 0; j < n; j++)
      temp += A[i * n + j] * x[j];
    y_aligned[i].value = temp;
  }

  return NULL;
}

void *Pth_mat_vect_with_barrier(void *rank)
{
  long my_rank = (long)rank;
  int local_m = m / thread_count;
  int my_first_row = my_rank * local_m;
  int my_last_row = my_first_row + local_m;

  for (int i = my_first_row; i < my_last_row; i++)
  {
    double temp = 0.0;
    for (int j = 0; j < n; j++)
      temp += A[i * n + j] * x[j];
    y[i] = temp;
    pthread_barrier_wait(&barrier);
  }

  return NULL;
}

/*------------------------------------------------------------------
 * Function:    Gen_matrix
 * Purpose:     Use random number generator to generate matrix entries
 * In args:     m, n
 * Out arg:     A
 */
void Gen_matrix(double A[], int m, int n)
{
  for (int i = 0; i < m; i++)
    for (int j = 0; j < n; j++)
      A[i * n + j] = random() / ((double)RAND_MAX);
}

/*------------------------------------------------------------------
 * Function:    Gen_vector
 * Purpose:     Use random number generator to generate vector entries
 * In arg:      n
 * Out arg:     x
 */
void Gen_vector(double x[], int n)
{
  for (int i = 0; i < n; i++)
    x[i] = random() / ((double)RAND_MAX);
}

/*------------------------------------------------------------------
 * Function:    Print_vector
 * Purpose:     Print a vector
 * In args:     title, y, m
 */
void Print_vector(char *title, double y[], double m)
{
  printf("%s\n", title);
  for (int i = 0; i < m; i++)
    printf("%6.3f ", y[i]);
  printf("\n");
}

/*------------------------------------------------------------------
 * Function:    Usage
 * Purpose:     Print usage message and exit
 * In arg:      prog_name
 */
void Usage(char *prog_name)
{
  fprintf(stderr, "usage: %s <thread_count> <m> <n>\n", prog_name);
  exit(0);
}
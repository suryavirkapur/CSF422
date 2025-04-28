#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <time.h>

int main() {
    float a[10][10], b[10], x[10], xn[10], sum, e;
    int i, j, n, flag = 0, key;
    double start_time, end_time;

    printf("\nThis program illustrates Gauss-Jacobi method to solve system of AX=B\n");
    printf("\nEnter the dimensions of coefficient matrix n: ");
    scanf("%d", &n);
    printf("\nEnter the elements of matrix A:\n");
    for (i = 0; i < n; i++) {
        // loop free from loop-carried dependance
        // best scheduling: static
        //#pragma omp parallel for schedule(static)  
        // no need for parallelizm here, single thread is best
        for (j = 0; j < n; j++) {
            scanf("%f", &a[i][j]);
        }
    }

    printf("\nEnter the elements of matrix B:\n");
    for (i = 0; i < n; i++) {
         // loop free from loop-carried dependance
        // best scheduling: static
         //#pragma omp parallel for schedule(static) 
         // no need for parallelism, single thread is best
        scanf("%f", &b[i]);
    }

    printf("\nThe system of linear equations:\n");
    for (i = 0; i < n; i++) {
        // loop free from loop-carried dependance
        // best scheduling: static
        //#pragma omp parallel for  // no need.  i/o is inherently sequential.
        printf("\n(%.2f)x1 + (%.2f)x2 + (%.2f)x3 = %.2f\n", a[i][0], a[i][1], a[i][2], b[i]); // corrected printf format
    }


    // check for diagonal dominance. 
    // best do outside any parallel regions
    for (i = 0; i < n; i++) {
        sum = 0;
        for (j = 0; j < n; j++) {
            sum += fabs(a[i][j]);
        }
        sum -= fabs(a[i][i]);
        if (fabs(a[i][i]) <= sum) { // corrected the condition
            flag = 1;
            break;
        }
    }

     if (flag == 1) {
        printf("\nThe system of linear equations are not diagonally dominant\n");
        return 0;
    }



    printf("\nThe system of linear equations are diagonally dominant\n");
    printf("\nEnter the initial approximations: ");
    for (i = 0; i < n; i++) {
        // loop free from loop-carried dependance
        // best scheduling is static
        //#pragma omp parallel for schedule(static) 
        //  no need for parallelism here single thread is sufficient.
        printf("\nx%d = ", (i + 1)); 
        scanf("%f", &x[i]);
    }
    printf("\nEnter the error tolerance level:\n");
    scanf("%f", &e);

    printf("x[1]\t\tx[2]\t\tx[3]");
    printf("\n");
    key = 0;

    start_time = omp_get_wtime();
    // gauss-jacobi iteration
    do {
        key = 0;

        // calculate the new approximations. this can be parallelized.
        #pragma omp parallel for private(sum) schedule(dynamic, 4)
        for (i = 0; i < n; i++) {
            sum = b[i];
            for (j = 0; j < n; j++) {
                if (i != j) {
                    sum -= a[i][j] * x[j];
                }
            }
            xn[i] = sum / a[i][i];
        }

         printf("\t%f\t %f\t %f\t", xn[0], xn[1], xn[2]); 

        // check for convergence. use a reduction.
        #pragma omp parallel for reduction(+:key)
        for (i = 0; i < n; i++) {
            if (fabs(xn[i] - x[i]) < e) {
                key++;
            }
        }



        #pragma omp parallel for
        for (i = 0; i < n; i++) {
            x[i] = xn[i];
        }
         printf("\n");

    } while (key < n);

    end_time = omp_get_wtime();

    printf("\nAn approximate solution to the given system of equations is\n");
    for (i = 0; i < n; i++) {
         // loop free from loop-carried dependance
         // best scheduling is static
         // #pragma omp parallel for schedule(static)
         // no real benefit from parallelizing this print loop again io is inherently sequantial
        printf("\nx[%d] = %f\n", (i + 1), x[i]);
    }
    printf("Time taken: %f seconds\n", end_time - start_time);
    return 0;
}
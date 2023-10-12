/*
CSED211 Cache Lab
Part A. Cache Simulator
--------------------
Name: Hyun June Kim
loginID: hyunjunekim
studentID: 20210643
--------------------
*/

/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    
    //row, column variable
    int row = 0;
    int column = 0;

    //each block's first index
    int eight_row, eight_column;

    //temporary variables
    int a0, a1, a2, a3, a4, a5, a6, a7;

    //case 32X32
    if ((M==32)&&(N == 32)) {

        for (eight_row = 0; eight_row < N; eight_row += 8) {
            for (eight_column = 0; eight_column < M; eight_column += 8) {

                for (row = eight_row; row < (eight_row + 8); row++) {
                    for (column = eight_column; column < (eight_column + 8); column++) {
                        
                        //consider the case of row index is equals to column index 
                        if (row == column) {
                            a0 = A[row][column];
                            a1 = row;
                        }
                        else {
                            B[column][row] = A[row][column];
                        }
                    }

                    //row=column case at outer loop
                    if (eight_column == eight_row) {
                        B[a1][a1] = a0;
                    }
                }
            
            }
        }
    }
    //case 64X64
    else if ((M==64)&&(N == 64)) {
        for (eight_row = 0; eight_row < N; eight_row += 8) {
            for (eight_column = 0; eight_column < M; eight_column += 8) {
                
                //upper half of A
                for (row = eight_row; row < (eight_row + 4); row++) {
                    
                    //temporary variables
                    a0 = A[row][eight_column];
                    a1 = A[row][eight_column + 1];
                    a2 = A[row][eight_column + 2];
                    a3 = A[row][eight_column + 3];
                    a4 = A[row][eight_column + 4];
                    a5 = A[row][eight_column + 5];
                    a6 = A[row][eight_column + 6];
                    a7 = A[row][eight_column + 7];
                    
                    //left-upper side
                    B[eight_column][row] = a0;
                    B[eight_column+1][row] = a1;
                    B[eight_column+2][row] = a2;
                    B[eight_column+3][row] = a3;

                    //right-upper side
                    B[eight_column][row+4] = a4;
                    B[eight_column+1][row + 4] = a5;
                    B[eight_column+2][row + 4] = a6;
                    B[eight_column+3][row + 4] = a7;
                }
                //lower half of A
                for (column = eight_column; column < (eight_column + 4); column++) {

                    //copying right-upper side of B
                    a0 = B[column][eight_row+4];
                    a1 = B[column][eight_row+5];
                    a2 = B[column][eight_row+6];
                    a3 = B[column][eight_row+7];

                    //copying left-lower side of A
                    a4 = A[eight_row+4][column];
                    a5 = A[eight_row+5][column];
                    a6 = A[eight_row+6][column];
                    a7 = A[eight_row+7][column];

                    //left-lower side of A to right-upper side of B
                    B[column][eight_row+4]=a4;
                    B[column][eight_row+5]=a5;
                    B[column][eight_row+6]=a6;
                    B[column][eight_row+7]=a7;

                    //right-upper side values to left-lower side
                    B[column+4][eight_row]=a0;
                    B[column+4][eight_row+1]=a1;
                    B[column+4][eight_row+2]=a2;
                    B[column+4][eight_row+3]=a3;

                }
                
                //right-lower part
                for(column = eight_column+4; column<(eight_column+8); column++){
                    for(row=eight_row+4;row<(eight_row+8);row++){
                        B[column][row]=A[row][column];
                    }
                }

            }
        }
    }
    //case 61X67
    else {
        for (eight_row = 0; eight_row < N; eight_row += 16) {
            for (eight_column = 0; eight_column < M; eight_column += 16) {
                for (row = eight_row; (row < N) && (row < eight_row + 16); row++) {

                    for (column = eight_column; (column < M) && (column < eight_column + 16); column++) {
                        
                        //consider the case of row index is equals to column index 
                        if (row == column) {
                            a0 = A[row][column];
                            a1 = row;
                        }
                        else {
                            B[column][row] = A[row][column];
                        }
                    }

                    //row=column case at outer loop
                    if (eight_column == eight_row) {
                        B[a1][a1] = a0;
                    }
                }
            }
        }
    }

    return;

}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}


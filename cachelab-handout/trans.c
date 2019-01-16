/*
Ellie Van De Graaff and Chandra Goodell
ellievdg and goodchan 
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
  int index = 0;
  int value = 0;
  if(N == 32 && M == 32){
    for(int i = 0; i < N; i+= 8){
      for(int j = 0; j < M; j+= 8){
	for(int k = i; (k < (i + 8)) && (k < N); k++){
	  for(int h = j; (h < (j + 8)) && (h < M); h++){
	    /* if(h == k){
	      continue;
	    }
	    else{
	      B[h][k] = A[k][h];
	    }
	    */
	     if(h != k){
	      B[h][k] = A[k][h];
	    }
	    
	    else{
	      index = k;
	      value = A[k][h];
	    }
	    
	  }
	  if(j == i){
	    B[index][index] = value;
	  }
	}
      }
    }
  }
  if(N == 67 && M == 61){
    int blockSize = 16;
    for(int i = 0; i < N; i += blockSize){
      for(int j = 0; j < M; j += blockSize){
	for(int x = i; ((x < N) && (x < i + blockSize)); ++x){
	    for(int y = j; ((y < M) && (y < j + blockSize)); ++y){
	       B[y][x] = A[x][y];
	    }
	  }
      }
    }
  }

  if(N == 64 && M == 64){
    int val1;
    int val2;
    int val3;
    int val4;
    int val5;
    int val6;
    int val7;
    int val8;
    for(int i = 0; i < N; i+=8){
      for(int j = 0; j < M; j+= 8){
	for(int x = i; x < (i + 4); x++){
	  val1 = A[x][j];
	  val2 = A[x][j + 1];
	  val3 = A[x][j + 2];
	  val4 = A[x][j + 3];
	  val5 = A[x][j + 4];
	  val6 = A[x][j + 5];
	  val7 = A[x][j + 6];
	  val8 = A[x][j + 7];
	  B[j][x] = val1;
	  B[j + 1][x] = val2;
	  B[j + 2][x] = val3;
	  B[j + 3][x] = val4;
	  B[j][x + 4] = val5;
	  B[j + 1][x + 4] = val6;
	  B[j + 2][x + 4] = val7;
	  B[j + 3][x + 4] = val8;
	}
	for(int x = (j + 4); x < (j + 8); x++){
	  val5 = A[i + 4][x - 4];
	  val6 = A[i + 5][x - 4];
	  val7 = A[i + 6][x - 4];
	  val8 = A[i + 7][x - 4];
	  val1 = B[x - 4][i + 4];
	  val2 = B[x - 4][i + 5];
	  val3 = B[x - 4][i + 6];
	  val4 = B[x - 4][i + 7];
	  B[x - 4][i + 4] = val5;
	  B[x - 4][i + 5] = val6;
	  B[x - 4][i + 6] = val7;
	  B[x - 4][i + 7] = val8;
	  B[x][i] = val1;
	  B[x][i + 1] = val2;
	  B[x][i + 2] = val3;
	  B[x][i + 3] = val4;
	  B[x][i + 4] = A[i + 4][x];
	  B[x][i + 5] = A[i + 5][x];
	  B[x][i + 6] = A[i + 6][x];
	  B[x][i + 7] = A[i + 7][x];
	}
      }
    }
  }
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


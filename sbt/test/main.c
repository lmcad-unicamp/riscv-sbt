#include <assert.h>
#include <stdio.h>

#define A_ROWS  3
#define A_COLS  3

#define B_ROWS  3
#define B_COLS  3

#define C_ROWS  A_ROWS
#define C_COLS  B_COLS

#define ELEM(M, i, j) \
  M[i * M##_COLS + j]

int A[A_ROWS * A_COLS] = {
  1, 2, 3,
  1, 2, 3,
  1, 2, 3
};

int B[B_ROWS * B_COLS] = {
  1, 2, 3,
  1, 2, 3,
  1, 2, 3
};

int C[C_ROWS * C_COLS];

int main()
{
  assert(A_COLS == B_ROWS);

  int i;
  int j;
  int k;

  for (i = 0; i < C_ROWS; i++)
    for (j = 0; j < C_COLS; j++)
      for (k = 0; k < A_COLS; k++)
        ELEM(C, i, j) += ELEM(A, i, k) * ELEM(B, k, j);

  for (i = 0; i < C_ROWS; i++)
    for (j = 0; j < C_COLS; j++)
      printf("C[%d,%d]=%d\n", i, j, ELEM(C, i, j));

  return 0;
}

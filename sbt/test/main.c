#include <stdio.h>

int A[] = {
  1, 1, 1,
  2, 2, 2,
  3, 3, 3
};

#define N sizeof(A) / sizeof(A[0])

int B[] = {
  1, 1, 1,
  1, 1, 1,
  1, 1, 1
};

int C[N];

int main()
{
  int i;

  for (i = 0; i < N; i++)
    C[i] = A[i] + B[i];

  for (i = 0; i < N; i++)
    printf("C[%d]=%d\n", i, C[i]);

  return 0;
}

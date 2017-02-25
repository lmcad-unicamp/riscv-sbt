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
int D[N];

int Y = 0;
int Z = 0;

int main()
{
  puts("Hello\n");
  /*
  int i;

  for (i = 0; i < N; i++)
    C[i] = A[i] + B[i];

  for (i = 0; i < N; i++)
    printf("C[%d]=%d\n", i, C[i]);

  printf("D[0]=%d\n", D[0]);
  D[2] = 2;
  printf("D[2]=%d\n", D[2]);

  //printf("Y=%d\n", Y);
  //printf("Z=%d\n", Z);
  //Z = 567;
  //printf("Z=%d\n", Z);
  */

  return 0;
}

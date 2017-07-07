#include <assert.h>
#include <stdio.h>

#define ROWS    300
#define COLS    ROWS

#define ELEM(M, i, j) \
    M[i * COLS + j]

int A[ROWS * COLS];
int B[ROWS * COLS];
int C[ROWS * COLS];

static void init()
{
    int i;
    int j;
    int n;
    int *m;

    for (n = 0; n < 2; n++) {
        m = n == 0? A : B;
        for (i = 0; i < ROWS; i++)
            for (j = 0; j < COLS; j++)
                ELEM(m, i, j) = i * ROWS + j;
    }
}

int main()
{
    int i;
    int j;
    int k;

    init();

    for (i = 0; i < ROWS; i++)
        for (j = 0; j < COLS; j++)
            for (k = 0; k < COLS; k++)
                ELEM(C, i, j) += ELEM(A, i, k) * ELEM(B, k, j);

    for (i = 0; i < ROWS; i++)
        for (j = 0; j < COLS; j++)
            printf("C[%d,%d]=%d\n", i, j, ELEM(C, i, j));

    return 0;
}

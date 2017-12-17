#include <assert.h>
#include <stdio.h>

#ifndef ROWS
#   define ROWS    500
#endif

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

    for (i = 0; i < ROWS; i++)
        for (j = 0; j < COLS; j++)
            ELEM(A, i, j) = j;

    for (i = 0; i < ROWS; i++)
        for (j = 0; j < COLS; j++)
            ELEM(B, i, j) = i;
}

static void print(char c, int *m)
{
    int i;
    int j;

    for (i = 0; i < ROWS; i++)
        for (j = 0; j < COLS; j++)
            printf("%c[%d,%d]=%d\n", c, i, j, ELEM(m, i, j));
}

int main()
{
    int i;
    int j;
    int k;

    init();

    print('A', A);
    print('B', B);

    for (i = 0; i < ROWS; i++)
        for (j = 0; j < COLS; j++)
            for (k = 0; k < COLS; k++)
                ELEM(C, i, j) += ELEM(A, i, k) * ELEM(B, k, j);

    print('C', C);
    return 0;
}

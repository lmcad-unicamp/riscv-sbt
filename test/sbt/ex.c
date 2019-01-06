#include <stdio.h>

int dot_prod(int a[2], int b[2])
{
    return a[0] * b[0] + a[1] * b[1];
}

int main(int argc, char **argv)
{
    int a[2] = {1 * argc, 2 * argc};
    int b[2] = {3 * argc, 4 * argc};

    printf("a.b = %i\n", dot_prod(a, b));
    return 0;
}

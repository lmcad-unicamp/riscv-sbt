#include <stdio.h>

int main()
{
    puts("printf test");
    printf("%f\n", 1.125);
    printf("%f %f\n", 1.2, 2.3);
    printf("%d %f %f\n", 10, 20.20, 30.30);
    printf("%d %d %f\n", 10, 20, 30.30);
    printf("%d %d %d %d %d\n", 10, 20, 30, 40, 50);
    return 0;
}

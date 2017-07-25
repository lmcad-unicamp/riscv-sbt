#include <stdio.h>

int main(int argc, char* argv[])
{
    int i;

    printf("argc=%d\n", argc);
    for (i = 0; i < argc; i++)
        printf("argv[%i]=%s\n", i, argv[i]);

    return 123;
}

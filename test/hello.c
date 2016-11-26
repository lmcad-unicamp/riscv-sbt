#include <stdio.h>
#include <unistd.h>

int main()
{
    printf("Hello, World!\n");
    write(1, "xxx\n", 4);
    return 0;
}

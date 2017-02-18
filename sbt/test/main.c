#include <stdio.h>

int main()
{
  printf("Hello, World!\n");
  printf("x=%d\n", 1234);
  perror("fake error");
  return 0;
}

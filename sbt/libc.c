#include <stdio.h>
#include <stdlib.h>

#define F(x) \
  void* rv32_##x = x;

F(abort)
F(exit)
F(fflush)
F(fopen)
F(free)
F(fprintf)
F(fscanf)
F(fwrite)
F(malloc)
F(perror)
F(printf)
F(putchar)
F(puts)

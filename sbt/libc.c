#include <stdio.h>
#include <stdlib.h>

#define F(x) \
  void* rv32_##x = x;

F(abort)
F(exit)
F(fclose)
F(ferror)
F(fflush)
F(fopen)
F(free)
F(fprintf)
F(fscanf)
F(fwrite)
F(_IO_getc)
F(malloc)
F(perror)
F(printf)
F(putchar)
F(puts)

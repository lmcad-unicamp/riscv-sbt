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
F(getc)
F(malloc)
F(perror)
F(printf)
F(putchar)
F(puts)

F(_IO_getc)

// x86 libc stuff
//int __isoc99_fscanf(FILE *__restrict, const char *__restrict, ...);
//F(__isoc99_fscanf)

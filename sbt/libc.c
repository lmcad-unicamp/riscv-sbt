#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define F(x) \
  void* rv32_##x = x;

F(abort)
F(exit)
F(fclose)
F(feof)
F(ferror)
F(fflush)
F(fgetpos)
F(fopen)
F(free)
F(fprintf)
F(fread)
F(fscanf)
F(fseek)
F(fwrite)
F(getc)
F(malloc)
F(memset)
F(perror)
F(printf)
F(putchar)
F(puts)
F(rand)

F(_IO_getc)
F(__ctype_toupper_loc)

// x86 libc stuff
//int __isoc99_fscanf(FILE *__restrict, const char *__restrict, ...);
//F(__isoc99_fscanf)

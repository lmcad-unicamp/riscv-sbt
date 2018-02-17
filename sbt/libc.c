#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// function

#define F(x) \
  void* rv32_##x = x;

F(abort)
F(atexit)
F(clock)
F(exit)
F(fclose)
F(feof)
F(ferror)
F(fflush)
F(fgetc)
F(fgetpos)
F(fopen)
F(free)
F(fprintf)
F(fputc)
F(fread)
F(fscanf)
F(fseek)
F(fwrite)
F(getc)
F(malloc)
F(memchr)
F(memcpy)
F(memset)
F(perror)
F(printf)
F(putchar)
F(puts)
F(rand)
F(read)
F(realloc)
F(strlen)
F(strncmp)
F(strtol)
F(tolower)
F(toupper)
F(write)

F(_IO_getc)
F(_IO_putc)
F(__ctype_tolower_loc)
F(__ctype_toupper_loc)


// math

F(acos)
F(atan)
F(cos)
F(pow)
F(sqrt)

// runtime

int sbt_printf_d(const char*, double);
F(sbt_printf_d)

// data

#define D(x) \
  void* rv32_##x = NULL;

#define DI(x) \
  rv32_##x = x;

D(stdin)
D(stderr)
D(stdout)

// dummy function to be able to reference non compile time stuff
void set_non_const()
{
    DI(stdin)
    DI(stderr)
    DI(stdout)
}

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

// function

#define F(x) \
  void* rv32_##x = x;

F(abort)
F(abs)
F(acos)
F(atan)
F(atexit)
F(atoi)
F(atof)
F(bcopy)
F(clock)
F(cos)
F(exit)
F(exp)
F(fclose)
F(feof)
F(ferror)
F(fflush)
F(fgetc)
F(fgetpos)
F(fgets)
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
F(pow)
F(printf)
F(putchar)
F(puts)
F(rand)
F(read)
F(realloc)
F(sin)
F(sleep)
F(sqrt)
F(sqrtf)
F(srand)
F(sscanf)
F(strlen)
F(strncmp)
F(strtod)
F(strtol)
F(tolower)
F(toupper)
F(usleep)
F(write)

F(_IO_getc)
F(_IO_putc)
F(__ctype_tolower_loc)
F(__ctype_toupper_loc)

// soft float

long double __extenddftf2(double);
F(__extenddftf2)

double __trunctfdf2(long double);
F(__trunctfdf2)

long double __addtf3(long double, long double);
F(__addtf3)

long double __subtf3(long double, long double);
F(__subtf3)

long double __multf3(long double, long double);
F(__multf3)

long double __divtf3(long double, long double);
F(__divtf3)

int __lttf2(long double, long double);
F(__lttf2)

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

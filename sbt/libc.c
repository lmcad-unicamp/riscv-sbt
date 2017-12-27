#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// function

#define F(x) \
  void* rv32_##x = x;

F(abort)
F(atexit)
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
F(tolower)
F(toupper)
F(write)

F(_IO_getc)
F(__ctype_tolower_loc)
F(__ctype_toupper_loc)


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

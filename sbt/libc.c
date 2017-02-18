#include <stdio.h>

#define F(x) (void *)&x

void *funcs[] = {
  F(perror),
  F(printf),
  F(puts),
  NULL
};


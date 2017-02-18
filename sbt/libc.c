#include <stdio.h>

#define F(x) (void *)&x

void *funcs[] = {
  F(printf),
  F(puts),
  NULL
};


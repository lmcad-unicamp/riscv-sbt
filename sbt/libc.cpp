#include <cstdio>

#define F(x) \
  auto rv32_##x = x;

F(fflush)
F(perror)
F(printf)
F(puts)

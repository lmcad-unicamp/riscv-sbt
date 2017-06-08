#include <cstdio>
#include <cstdlib>

#define F(x) \
  auto rv32_##x = x;

F(abort)
F(fflush)
F(perror)
F(printf)
F(puts)

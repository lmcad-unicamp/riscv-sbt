#include "Runtime.h"

#include <math.h>
#include <stdio.h>

int sbt_printf_d(const char *fmt, double d)
{
    return printf(fmt, d);
}

#ifdef __i386__

#define push_fp128(i) \
    "pushl 12(%" #i ")\n\t" \
    "pushl  8(%" #i ")\n\t" \
    "pushl  4(%" #i ")\n\t" \
    "pushl   (%" #i ")\n\t"

#define push_rvptr(i) \
    "pushl $0\n\t" \
    "pushl $0\n\t" \
    "pushl $0\n\t" \
    "pushl %" #i "\n\t"

#define call2(fn, r, a, b) \
    asm volatile ( \
        push_fp128(2) \
        push_fp128(1) \
        push_rvptr(0) \
        "calll " #fn "\n\t" \
        "addl $0x2c, %%esp" \
        : \
        : "r" (r), "r" (a), "r" (b) \
        : "memory" \
    )


void sbt__addtf3(fp128 *r, fp128 *a, fp128 *b)
{
    call2(__addtf3, r, a, b);
}

void sbt__subtf3(fp128 *r, fp128 *a, fp128 *b)
{
    call2(__subtf3, r, a, b);
}

void sbt__multf3(fp128 *r, fp128 *a, fp128 *b)
{
    call2(__multf3, r, a, b);
}

void sbt__divtf3(fp128 *r, fp128 *a, fp128 *b)
{
    call2(__divtf3, r, a, b);
}

#endif

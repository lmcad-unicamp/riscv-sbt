#include "Runtime.h"

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


#ifdef __i386__

#include <math.h>

double __trunctfdf2(__float128);
__float128 __extenddftf2(double);
__float128 __addtf3(__float128, __float128);
__float128 __subtf3(__float128, __float128);
__float128 __multf3(__float128, __float128);
__float128 __divtf3(__float128, __float128);
int __lttf2(__float128, __float128);

#define push_fp128(i) \
    "pushl 12(%" #i ")\n\t" \
    "pushl  8(%" #i ")\n\t" \
    "pushl  4(%" #i ")\n\t" \
    "pushl   (%" #i ")\n\t"

#define push_ptr(i) \
    "pushl %" #i "\n\t"

#define push_ptr_a16(i) \
    "pushl $0\n\t" \
    "pushl $0\n\t" \
    "pushl $0\n\t" \
    push_ptr(i)

#define calltf3(fn, r, a, b) \
    asm volatile ( \
        push_fp128(2) \
        push_fp128(1) \
        push_ptr_a16(0) \
        "calll " #fn "\n\t" \
        "addl $0x2c, %%esp" \
        : \
        : "r" (r), "r" (a), "r" (b) \
        : "memory" \
    )


#define decl_fn(fn) \
    ".globl " #fn "\n" \
    #fn ":\n\t"


#define enter \
    "pushl %ebp\n\t" \
    "movl %esp, %ebp\n\t"

#define leave \
    "popl %ebp\n\t" \
    "ret\n"

// void sbt__extenddftf2(double2 *r, double a)
asm(
    decl_fn(sbt__extenddftf2)
    enter
    "pushl 0x10(%ebp)\n\t"
    "pushl 0x0c(%ebp)\n\t"
    "pushl 0x08(%ebp)\n\t"
    "calll __extenddftf2\n\t"
    "addl $0x8, %esp\n\t"
    leave
);

// double sbt__trunctfdf2(double2 *a)
asm(
    decl_fn(sbt__trunctfdf2)
    enter
    "movl  0x8(%ebp), %eax\n\t"
    "pushl 0xc(%eax)\n\t"
    "pushl 0x8(%eax)\n\t"
    "pushl 0x4(%eax)\n\t"
    "pushl 0x0(%eax)\n\t"
    "calll __trunctfdf2\n\t"
    "addl $0x10, %esp\n\t"
    leave
);

void sbt_test()
{
    __float128 ta;
    double da;

    da = 1.5;
    ta = __extenddftf2(da);
    da = __trunctfdf2(ta);
}

void sbt__addtf3(double2 *r, double2 *a, double2 *b)
{
    calltf3(__addtf3, r, a, b);
}

void sbt__subtf3(double2 *r, double2 *a, double2 *b)
{
    calltf3(__subtf3, r, a, b);
}

void sbt__multf3(double2 *r, double2 *a, double2 *b)
{
    calltf3(__multf3, r, a, b);
}

void sbt__divtf3(double2 *r, double2 *a, double2 *b)
{
    calltf3(__divtf3, r, a, b);
}

// int sbt__lttf2(double2 *a, double2 *b)
asm(
    decl_fn(sbt__lttf2)
    enter
    "movl  0xc(%ebp), %eax\n\t"
    "pushl 0xc(%eax)\n\t"
    "pushl 0x8(%eax)\n\t"
    "pushl 0x4(%eax)\n\t"
    "pushl 0x0(%eax)\n\t"
    "movl  0x8(%ebp), %eax\n\t"
    "pushl 0xc(%eax)\n\t"
    "pushl 0x8(%eax)\n\t"
    "pushl 0x4(%eax)\n\t"
    "pushl 0x0(%eax)\n\t"
    "calll __lttf2\n\t"
    "addl $0x20, %esp\n\t"
    leave
);

#endif  // __i386__


void sbtabort()
{
    kill(getpid(), SIGABRT);
}


int sbt_printf_d(const char *fmt, double d)
{
    return printf(fmt, d);
}

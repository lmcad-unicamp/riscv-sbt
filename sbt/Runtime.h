#ifndef SBT_RUNTIME_H
#define SBT_RUNTIME_H

int sbt_printf_d(const char*, double);

// soft float

#ifdef __i386__
typedef __float128 fp128;
#else   // riscv
typedef long double fp128;
#endif

typedef struct double2_s {
    double d0;
    double d1;
} double2;

double sbt__trunctfdf2(double2 *a);
void sbt__extenddftf2(double2 *r, double a);
void sbt__addtf3(double2 *r, double2 *a, double2 *b);
void sbt__subtf3(double2 *r, double2 *a, double2 *b);
void sbt__multf3(double2 *r, double2 *a, double2 *b);
void sbt__divtf3(double2 *r, double2 *a, double2 *b);
int  sbt__lttf2(double2 *a, double2 *b);

#endif

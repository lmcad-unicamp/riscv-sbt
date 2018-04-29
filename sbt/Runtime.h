#ifndef SBT_RUNTIME_H
#define SBT_RUNTIME_H

int sbt_printf_d(const char*, double);

// soft float

#ifdef __i386__
typedef __float128 fp128;
#else   // riscv
typedef long double fp128;
#endif

double __trunctfdf2(fp128);
fp128 __extenddftf2(double);
fp128 __addtf3(fp128, fp128);
fp128 __subtf3(fp128, fp128);
fp128 __multf3(fp128, fp128);
fp128 __divtf3(fp128, fp128);
int __lttf2(fp128, fp128);

void sbt__addtf3(fp128 *r, fp128 *a, fp128 *b);
void sbt__subtf3(fp128 *r, fp128 *a, fp128 *b);
void sbt__multf3(fp128 *r, fp128 *a, fp128 *b);
void sbt__divtf3(fp128 *r, fp128 *a, fp128 *b);

#endif

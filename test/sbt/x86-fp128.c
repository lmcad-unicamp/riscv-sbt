#include <Runtime.h>

#include <math.h>
#include <stdio.h>

double __trunctfdf2(__float128);
__float128 __extenddftf2(double);

int main()
{
    double dr;
    double2 tr;
    double2 ta;
    double2 tb;
    double da, db;

    da = 2.5;
    db = 3.0;

    printf("&tr=0x%08X\n", (unsigned)&tr);
    printf("&ta=0x%08X, &tb=0x%08X\n", (unsigned)&ta, (unsigned)&tb);

    sbt__extenddftf2(&ta, da);
    sbt__extenddftf2(&tb, db);

    sbt__addtf3(&tr, &ta, &tb);
    dr = sbt__trunctfdf2(&tr);
    printf("add: %f\n", dr);

    sbt__subtf3(&tr, &ta, &tb);
    dr = sbt__trunctfdf2(&tr);
    printf("sub: %f\n", dr);

    sbt__multf3(&tr, &ta, &tb);
    dr = sbt__trunctfdf2(&tr);
    printf("mul: %f\n", dr);

    sbt__divtf3(&tr, &ta, &tb);
    dr = sbt__trunctfdf2(&tr);
    printf("div: %f\n", dr);

    printf("a<b: %d\n", sbt__lttf2(&ta, &tb));
    printf("b<a: %d\n", sbt__lttf2(&tb, &ta));
    return 0;
}

#include <Runtime.h>

#include <math.h>
#include <stdio.h>

int main()
{
    fp128 tr, ta, tb;
    double dr, da, db;

    da = 2.5;
    db = 3.0;

    ta = __extenddftf2(da);
    tb = __extenddftf2(db);

    sbt__addtf3(&tr, &ta, &tb);
    dr = __trunctfdf2(tr);
    printf("add: %f\n", dr);

    sbt__subtf3(&tr, &ta, &tb);
    dr = __trunctfdf2(tr);
    printf("sub: %f\n", dr);

    sbt__multf3(&tr, &ta, &tb);
    dr = __trunctfdf2(tr);
    printf("mul: %f\n", dr);

    sbt__divtf3(&tr, &ta, &tb);
    dr = __trunctfdf2(tr);
    printf("div: %f\n", dr);

    return 0;
}

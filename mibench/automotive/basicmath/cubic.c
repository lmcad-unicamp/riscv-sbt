#include <math.h>

double func(double a)
{
    if (a > 10)
        a = sin(a);
    else
        a = cos(a);
    return a + 35;
}

/*
void func(double a)
{
    a = sqrt(a)/a;
}
*/

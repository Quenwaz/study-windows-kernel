#include "dll_hijacking_org.hpp"
#include <cstdio>

_declspec(dllimport) double some_func_b(double a1, double a2);


DLL_HIJACKING_API double some_func_a(double a1, int a2, int a3)
{
    fprintf(stderr, "dll_hijacking_org::some_func_a\n");
    some_func_b(1,2);
    return a1 * a2 / a3;
}


DLL_HIJACKING_API double some_func_c(double a1)
{
    return a1*a1;
}
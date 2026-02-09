#ifndef _MATH_H
#define _MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

inline static double trunc (double t) {
    typedef double (*fn_ptr_t)(double);
    return ((fn_ptr_t)_sys_table_ptrs[200])(t);
}

inline static double floor (double t) {
    typedef double (*fn_ptr_t)(double);
    return ((fn_ptr_t)_sys_table_ptrs[201])(t);
}

inline static double pow (double x, double y) {
    typedef double (*fn_ptr_t)(double, double);
    return ((fn_ptr_t)_sys_table_ptrs[202])(x, y);
}
inline static float powf(float x, float y) {
    typedef float (*fn)(float, float);
    return ((fn)_sys_table_ptrs[257])(x, y);
}

inline static double sqrt (double x) {
    typedef double (*fn_ptr_t)(double);
    return ((fn_ptr_t)_sys_table_ptrs[203])(x);
}

inline static double sin (double x) {
    typedef double (*fn_ptr_t)(double);
    return ((fn_ptr_t)_sys_table_ptrs[204])(x);
}

inline static double cos (double x) {
    typedef double (*fn_ptr_t)(double);
    return ((fn_ptr_t)_sys_table_ptrs[205])(x);
}

inline static double tan (double x) {
    typedef double (*fn_ptr_t)(double);
    return ((fn_ptr_t)_sys_table_ptrs[206])(x);
}
inline static double atan (double x) {
    typedef double (*fn_ptr_t)(double);
    return ((fn_ptr_t)_sys_table_ptrs[207])(x);
}
inline static double log (double x) {
    typedef double (*fn_ptr_t)(double);
    return ((fn_ptr_t)_sys_table_ptrs[208])(x);
}
inline static double exp (double x) {
    typedef double (*fn_ptr_t)(double);
    return ((fn_ptr_t)_sys_table_ptrs[209])(x);
}

#include <stdint.h>

inline static
double copysign(double x, double y)
{
    union {
        double d;
        uint64_t u;
    } ux = { x }, uy = { y };

    ux.u &= UINT64_C(0x7FFFFFFFFFFFFFFF);
    ux.u |= uy.u & UINT64_C(0x8000000000000000);

    return ux.d;
}

inline static
float copysignf(float x, float y)
{
    union {
        float f;
        uint32_t u;
    } ux = { x }, uy = { y };

    ux.u &= 0x7FFFFFFFu;
    ux.u |= uy.u & 0x80000000u;

    return ux.f;
}

inline static
double fabs(double x) {
    return copysign(x, 1.0);
}

inline static 
float fabsf(float x) {
    return copysignf(x, 1.0f);
}

inline static 
long double fabsl(long double x) {
    return copysign(x, 1.0);
}

#ifdef __cplusplus
}
#endif

#endif // _MATH_H

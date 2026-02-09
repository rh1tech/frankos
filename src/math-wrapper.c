#include <math.h>
#include "math-wrapper.h"

uint32_t __u32u32u32_div(uint32_t x, uint32_t y) {
    return x / y;
}

uint32_t __u32u32u32_rem(uint32_t x, uint32_t y) {
    return x % y;
}

float __fff_div(float x, float y) {
    return x / y;
}
float __fff_mul(float x, float y) {
    return x * y;
}
float __ffu32_mul(float x, uint32_t y) {
    return x * y;
}
float __ffu32_div(float x, uint32_t y) {
    return x / y;
}

double __ddd_div(double x, double y) {
    return x / y;
}
double __ddd_mul(double x, double y) {
    return x * y;
}
double __ddu32_mul(double x, uint32_t y) {
    return x * y;
}
double __ddu32_div(double x, uint32_t y) {
    return x / y;
}
double __ddf_mul(double x, float y) {
    return x * y;
}

double __trunc (double t) {
    return trunc(t);
}
double __floor (double t) {
    return floor(t);
}
double __pow (double x, double y) {
    return pow(x, y);
}
double __sqrt (double x) {
    return sqrt(x);
}
double __sin (double a) {
    return sin(a);
}
double __cos (double a) {
    return sin(a);
}
double __tan (double a) {
    return tan(a);
}
double __atan (double x) {
    return atan(x);
}
double __log (double x) {
    return log(x);
}
double __exp (double x) {
    return exp(x);
}

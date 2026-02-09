// use it to resolve issues like memset and/or memcpy are not found on elf32 obj execution attempt
#include "m-os-api.h"
float __aeabi_fmul(float x, float y) { //         single-precision multiplication
    typedef float (*fn)(float, float);
    return ((fn)_sys_table_ptrs[210])(x, y);
}

float __aeabi_i2f(int x) { //                   integer to float (single precision) conversion
    typedef float (*fn)(int);
    return ((fn)_sys_table_ptrs[211])(x);
}

float __aeabi_fadd(float x, float y) { //         single-precision addition
    typedef float (*fn)(float, float);
    return ((fn)_sys_table_ptrs[212])(x, y);
}

float __aeabi_fsub(float x, float y) { //         single-precision subtraction
    typedef float (*fn)(float, float);
    return ((fn)_sys_table_ptrs[213])(x, y);
}

float __aeabi_fdiv(float n, float d) { //    single-precision division, n / d
    typedef float (*fn)(float, float);
    return ((fn)_sys_table_ptrs[214])(n, d);
}

int __aeabi_fcmpge(float a, float b) { //        result (1, 0) denotes (>=, ?<) [2], use for C >=
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[215])(a, b);
}

int __aeabi_idivmod(int x, int y) {
    typedef int (*fn)(int, int);
    return ((fn)_sys_table_ptrs[216])(x, y);
}
int __aeabi_idiv(int x, int y) {
    typedef int (*fn)(int, int);
    return ((fn)_sys_table_ptrs[217])(x, y);
}
double __aeabi_f2d(float x) {
    typedef double (*fn)(float);
    return ((fn)_sys_table_ptrs[218])(x);
}
float __aeabi_d2f(double x) {
    typedef float (*fn)(double);
    return ((fn)_sys_table_ptrs[219])(x);
}
int __aeabi_f2iz(float x) { //                     float (single precision) to integer C-style conversion [3]
    typedef int (*fn)(float);
    return ((fn)_sys_table_ptrs[220])(x);
}
int __aeabi_fcmplt(float x, float y) { //        result (1, 0) denotes (<, ?>=) [2], use for C <
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[221])(x, y);
}
double __aeabi_dsub(double x, double y) { //     double-precision subtraction, x - y
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[222])(x, y);
}
int __aeabi_d2iz(double x) { //                     double (double precision) to integer C-style conversion [3]
    typedef int (*fn)(double);
    return ((fn)_sys_table_ptrs[223])(x);
}
int __aeabi_fcmpeq(float x, float y) { //         result (1, 0) denotes (=, ?<>) [2], use for C == and !=
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[224])(x, y);
}
int __aeabi_fcmpun(float x, float y) { //         result (1, 0) denotes (?, <=>) [2], use for C99 isunordered()
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[225])(x, y);
}
int __aeabi_fcmpgt(float x, float y) { //         result (1, 0) denotes (>, ?<=) [2], use for C >
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[226])(x, y);
}
int __aeabi_dcmpge(double x, double y) { //         result (1, 0) denotes (>=, ?<) [2], use for C >=
    typedef int (*fn)(double, double);
    return ((fn)_sys_table_ptrs[227])(x, y);
}
unsigned __aeabi_uidiv(unsigned x, unsigned y) {
    typedef int (*fn)(unsigned, unsigned);
    return ((fn)_sys_table_ptrs[228])(x, y);
}
float __aeabi_ui2f(unsigned x) {
    typedef float (*fn)(unsigned);
    return ((fn)_sys_table_ptrs[229])(x);
}
unsigned __aeabi_f2uiz(float x) { //             float (single precision) to unsigned C-style conversion [3]
    typedef unsigned (*fn)(float);
    return ((fn)_sys_table_ptrs[230])(x);
}
int __aeabi_fcmple(float x, float y) { //         result (1, 0) denotes (<=, ?>) [2], use for C <=
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[231])(x, y);
}
unsigned __aeabi_uidivmod(unsigned x, unsigned y) {
    typedef int (*fn)(unsigned, unsigned);
    return ((fn)_sys_table_ptrs[228])(x, y);
}
double __aeabi_dmul(double x, double y) {
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[245])(x, y);
}
double __aeabi_ddiv(double x, double y) {
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[246])(x, y);
}
double __aeabi_dadd(double x, double y) {
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[247])(x, y);
}
double __aeabi_i2d(int x) {
    typedef double (*fn)(int);
    return ((fn)_sys_table_ptrs[248])(x);
}
double __aeabi_dcmpeq(double x, double y) {
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[249])(x, y);
}
double __aeabi_ui2d(unsigned x) {
    typedef double (*fn)(unsigned);
    return ((fn)_sys_table_ptrs[250])(x);
}
double __aeabi_dcmplt(double x, double y) {
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[251])(x, y);
}
double __aeabi_dcmpgt(double x, double y) {
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[251])(y, x);
}
unsigned __aeabi_d2uiz(double x) {
    typedef unsigned (*fn)(double);
    return ((fn)_sys_table_ptrs[256])(x);
}
int __clzsi2 (unsigned int a ) {
    typedef int (*fn)(unsigned int);
    return ((fn)_sys_table_ptrs[258])(a);
}
long long __aeabi_lmul(long long x, long long y) {
    typedef long long (*fn)(long long, long long);
    return ((fn)_sys_table_ptrs[259])(x, y);
}
unsigned long long __aeabi_uldivmod(unsigned long long x, unsigned long long y) {
    typedef unsigned long long (*fn)(unsigned long long, unsigned long long);
    return ((fn)_sys_table_ptrs[264])(x, y);
}

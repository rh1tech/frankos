/*
 * Originally from Murmulator OS 2 by DnCraptor
 * https://github.com/DnCraptor/murmulator-os2
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UF2_MODE
#ifndef NEW_IMPL
#define NEW_IMPL
inline void* operator new(size_t sz) {
    return malloc(sz);
}
inline void operator delete(void* p) {
    return free(p);
}
inline void operator delete(void* p, size_t) {
    return free(p);
}
inline void* operator new[](size_t sz) {
    return malloc(sz);
}
inline void operator delete[](void* p) {
    return free(p);
}
inline void operator delete[](void* p, size_t) {
    return free(p);
}
#endif
#endif

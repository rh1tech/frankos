#pragma once

template<typename T> class unordered_set {
    size_t sz;
    size_t alloc;
    T* p;
public:
    inline unordered_set(void): sz(0), alloc(16), p(new T[16]) { }
    inline ~unordered_set(void) { delete[] p; }
    inline size_t size(void) const { return this->sz; }
    inline void clear(void) { sz = 0; }
    class iterator {};
};

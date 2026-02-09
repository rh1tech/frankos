#pragma once

template<typename T> class vector {
    size_t sz;
    size_t alloc;
    T* p;
public:
    inline vector(void): sz(0), alloc(16), p((T*)calloc(sizeof(T), alloc)) { }
    inline vector(size_t al): sz(0), alloc(al), p((T*)calloc(sizeof(T), alloc)) { }
    inline vector(size_t sz, T init_by): sz(sz), alloc(sz), p((T*)calloc(sizeof(T), alloc)) {
        for (size_t i = 0; i < sz; ++i) {
            p[i] = init_by;
        }
    }
    inline ~vector(void) { delete[] p; }
    inline size_t size(void) const { return this->sz; }
    inline const T& operator[](size_t a) const { return p[a]; }
    inline T& operator[](size_t a) { return p[a]; }
    inline const T* data(void) const { return p;}
    inline T* data(void) { return p;}
    inline void clear(void) { sz = 0; }
    inline void resize(size_t size) {
        if (size > sz) {
            /// TODO:
            return;
        }
        sz = size;
    }
    inline void push_back(const T& t) {
        if (sz == alloc) {
            alloc++;
            T* p1 = (T*)calloc(sizeof(T), alloc);
            memcpy(p1, p, sz * sizeof(T));
            free(p);
            p = p1;
        }
        p[sz++] = t;
    }
    class iterator {
        T* p;
    public:
        iterator& operator++() { // prefix
            ++p;
            return *this;
        }
        iterator& operator++(int) { // postfix
            iterator cp = *this;
            p++;
            return cp;
        }
        T& operator*() {
            return *p;
        }
    };
};

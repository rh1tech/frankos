#pragma once
#ifndef m_os_api_psram
#define m_os_api_psram
#include "m-os-api.h"

class uint8_i {
    size_t base;
public:
    inline uint8_i(void) : base(0) {}
    inline uint8_i(size_t b) : base(b) {}
    inline uint8_i(const uint8_i& o) : base(o.base) {}
    inline void write(size_t a, uint8_t v) const { write8psram(base + a, v); }
    inline void writeNshift(uint8_t v) { write8psram(base, v); ++base; }
    inline void write(size_t a, uint32_t v) const { write32psram(base + a, v); }
    inline void writeNshift(uint32_t v) { write32psram(base, v); base += 4; }
    template<typename IA> inline uint8_i operator+(IA off) { return uint8_i(base + off); }
    template<typename IA> inline uint8_i operator-(IA off) { return uint8_i(base - off); }
    inline uint8_i operator[](size_t a) { return uint8_i(base + a); }
    inline uint8_t get() { return read8psram(base); }
    inline uint32_t get32(size_t a) const { return read32psram(base + a); }
    inline uint8_i& operator&() { return *this; } // no use-cases to get address of (pointer to) this object
    inline uint8_i& operator*() { return *this; } // no use-cases to get address of (pointer to) this object
    inline operator uint8_t() { return get(); }
    inline void operator=(uint8_t v) { write(0, v); }
    inline uint8_i operator++() { return *this + 1;}
    inline uint8_i operator++(int) { uint8_i t = *this; ++base; return t; }
    inline uint8_i operator--() { return *this - 1;}
    inline uint8_i operator--(int) { uint8_i t = *this; --base; return t; }
    inline uint8_i operator+=(size_t sz) { return *this + sz;}
    inline uint8_i operator-=(size_t sz) { return *this - sz;}
    inline bool operator==(int p) const { return base == (size_t)p; }
    inline bool operator!=(int p) const { return base != (size_t)p; }
    inline bool operator==(const uint8_i& p) const { return base == p.base; }
    inline bool operator!=(const uint8_i& p) const { return base != p.base; }
    inline uint8_i operator|=(uint8_t v) { write8psram(base, get() | v); return *this; }
    inline uint8_i operator&=(uint8_t v) { write8psram(base, get() & v); return *this; }
};

static void memset(uint8_i p, uint8_t v, size_t sz) {
    for (size_t i = 0; i < sz; ++i) {
        p.write(i, v);
    }
}

class psram_manager {
    static size_t off;
public:
    inline static size_t get_base(size_t sz) {
        size_t res = off;
        off += sz;
        return res;
    }
    inline static void release_base(size_t base) {
        // TODO: release;
    }
};
size_t psram_manager::off = 0;

class psram {
    size_t base;
public:
    inline psram(size_t sz) : base( psram_manager::get_base(sz) ) { }
    inline ~psram(void) { psram_manager::release_base(base); }
    inline uint8_i operator[](size_t a) { return uint8_i(base + a); }
    inline operator uint8_i() { return uint8_i(base); }
    template<typename IA> inline uint8_i operator+(IA off) { return uint8_i(base + off); }
};

class psram_a2 {
    size_t sz1;
    size_t sz2;
    size_t base;
public:
    inline psram_a2(size_t sz1, size_t sz2) : sz1(sz1), sz2(sz2), base( psram_manager::get_base(sz1 * sz2) ) { }
    inline ~psram_a2(void) { psram_manager::release_base(base); }
    inline uint8_i operator[](size_t a) { return uint8_i(base + a * sz2); }
};

static size_t fread(const uint8_i psram, size_t sz1, size_t sz2, FIL* file) {
    size_t res = 0;
    sz2 *= sz1;
    uint32_t* buf = new uint32_t[128]();
    while (res < sz2 && FR_OK == f_read(file, buf, 128 << 2, &sz1)) {
        for (size_t i = 0; i < sz1; i += 4) {
            psram.write(res + i, buf[i >> 2]);
        }
        res += sz1;
    }
    delete[] buf;
    return res;
}
static size_t fwrite(const uint8_i psram, size_t sz1, size_t sz2, FIL* file) {
    size_t res = 0;
    sz2 *= sz1;
    uint32_t* buf = new uint32_t[128]();
    for (size_t i = 0; i < 512 && res + i < sz2; i += 4) {
        buf[i >> 2] = psram.get32(res + i);
    }
    while (res < sz2 && FR_OK == f_write(file, buf, sz2 > res + 512 ? 512 : sz2 - res, &sz1)) {
        res += sz1;
        if (res == sz2) break;
        for (size_t i = 0; i < 512 && res + i < sz2; i += 4) {
            buf[i >> 2] = psram.get32(res + i);
        }
    } 
    delete[] buf;
    return res;
}

template<typename T> class uint8_iT {
    size_t base;
public:
    inline uint8_iT(void) : base(0) {}
    inline uint8_iT(size_t b) : base(b) {}
    inline uint8_iT(const uint8_iT& o) : base(o.base) {}
    inline void write(size_t a, const T& v) const {
        const char* c = (const char*)&v;
        size_t b = base + a * sizeof(T);
        for(size_t i = 0; i < sizeof(T); ++i) {
            write8psram(b + i, c[i]);
        }
    }
    template<typename IA> inline uint8_iT operator+(IA off) { return uint8_iT(base + off * sizeof(T)); }
    inline uint8_iT operator[](size_t a) { return uint8_i(base + a * sizeof(T)); }
    inline T get() {
        T res;
        char* c = (char*)&res;
        size_t b = base;
        for(size_t i = 0; i < sizeof(T); ++i) {
            c[i] + read8psram(b + i);
        }
        return res;
    }
    inline uint8_iT& operator&() { return *this; } // no use-cases to get address of (pointer to) this object
    inline operator T() { return get(); }
    inline void operator=(const T& v) { write(0, v); }
};

template<typename T> class psramT {
    size_t base;
public:
    inline psramT(size_t sz) : base( psram_manager::get_base(sz * sizeof(T)) ) { }
    inline ~psramT(void) { psram_manager::release_base(base); }
    inline uint8_iT<T> operator[](size_t a) { return uint8_iT<T>(base + a); }
    inline operator uint8_iT<T>() { return uint8_iT<T>(base); }
    template<typename IA> inline uint8_iT<T> operator+(IA off) { return uint8_iT<T>(base + off); }
};


#endif

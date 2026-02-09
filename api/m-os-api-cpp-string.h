#pragma once

static inline bool isspace(char c) {
    return c == ' ' || c == '\t';
}

class string {
    size_t sz;
    size_t alloc;
    char* p;
public:
    inline string(): sz(1), alloc(16), p(new char[16]) { p[0] = 0; }
    inline ~string() { delete[] p; }
    inline string(const char* s) {
        sz = strlen(s) + 1;
        alloc = sz + 16;
        p = new char[alloc];
        strncpy(p, s, sz);
    }
    inline string(const string& s): sz(s.sz), alloc(sz + 1), p(new char[alloc]) {
        strncpy(p, s.p, sz);
    }
    inline string(char* cs, size_t size): sz(size), alloc(size + 1), p(new char[alloc]) {
        strncpy(p, cs, sz);
    }
    inline const char* c_str() const { return p; }
    inline string& operator=(const string& s) {
        if (this == &s) {
            return *this;
        }
        delete[] p;
        sz = s.sz;
        alloc = s.alloc;
        p = new char[alloc];
        strncpy(p, s.p, sz);
        return *this;
    }
    inline string& operator=(const char* s) {
        sz = strlen(s) + 1;
        if (sz < alloc) {
            strncpy(p, s, sz);
            return *this;
        }
        delete[] p;
        alloc = sz + 16;
        p = new char[alloc];
        strncpy(p, s, sz);
        return *this;
    }
    inline char operator[](size_t i) const {
        return p[i];
    }
    inline size_t size() const { return sz - 1; }
    inline string& operator+=(const char c) {
        if (sz < alloc) {
            p[sz - 1] = c;
            p[sz++] = 0;
            return *this;
        }
        alloc += 16;
        char* n_p = new char[alloc];
        strncpy(n_p, p, sz);
        delete[] p;
        p = n_p;
        p[sz - 1] = c;
        p[sz++] = 0;
        return *this;
    }
    inline string& operator+=(const char* s) {
        size_t n_sz = strlen(s);
        if (sz + n_sz < alloc) {
            strncpy(p + sz - 1, s, n_sz + 1);
            sz += n_sz;
            return *this;
        }
        alloc += n_sz + 16;
        char* n_p = new char[alloc];
        strncpy(n_p, p, sz);
        delete[] p;
        p = n_p;
        strncpy(p + sz - 1, s, n_sz + 1);
        sz += n_sz;
        return *this;
    }
    inline string& operator+=(const string& s) {
        if (sz + s.sz - 1 < alloc) {
            strncpy(p + sz - 1, s.p, s.sz);
            sz += s.sz - 1;
            return *this;
        }
        alloc += s.sz + 16;
        char* n_p = new char[alloc];
        strncpy(n_p, p, sz);
        delete[] p;
        p = n_p;
        strncpy(p + sz - 1, s.p, s.sz);
        sz += s.sz - 1;
        return *this;
    }
    inline string& operator+(const string& s2) {
        *this += s2;
        return *this;
    }
    inline friend string operator+(const string& s1, const string& s2) {
        string s(s1);
        s += s2;
        return s;
    }
    inline void ltrim(void) {
        size_t i = 0;
        for (; i < sz; ++i) {
            if (!isspace(p[i])) {
                break;
            }
        }
        if (i > 0) {
            char* pi = p;
            for (size_t j = i; j <= sz; ++j) {
                *pi++ = p[j];
            }
            sz -= i;
        }
    }
    inline void rtrim(void) {
        for (int i = sz - 1; i >= 0; --i) {
            if (!isspace(p[i])) {
                p[i] = 0;
                sz = i;
                return;
            }
        }
    }
};

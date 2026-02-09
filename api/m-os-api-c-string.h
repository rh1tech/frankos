#ifndef M_API_C_STRING
#define M_API_C_STRING

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "task.h"

typedef struct string {
    size_t size; // string size excluding end-0
    size_t alloc; // really allocated bytes
    char* p;
} string_t;

static string_t* new_string_v(void) {
    string_t* res = (string_t*)pvPortMalloc(sizeof(string_t));
    res->size = 0,
    res->alloc = 16;
    res->p = (char*)pvPortMalloc(16);
    res->p[0] = 0;
    return res;
}

static string_t* string_subsrt(string_t* s, size_t start, size_t len) {
    if (s->size < start) return NULL;
    string_t* res = (string_t*)pvPortMalloc(sizeof(string_t));
    res->alloc = len + 1;
    res->p = (char*)pvPortMalloc(len + 1);
    strncpy(res->p, s->p + start, len);
    res->size = strlen(res->p);
    res->p[len] = 0;
    return res;
}

static void delete_string(string_t* s) {
    vPortFree(s->p);
    vPortFree(s);
}

static string_t* new_string_cc(const char* s) {
    string_t* res = (string_t*)pvPortMalloc(sizeof(string_t));
    res->size = strlen(s);
    res->alloc = res->size + 1;
    res->p = (char*)pvPortMalloc(res->alloc);
    strncpy(res->p, s, res->alloc);
    return res;
}

static string_t* new_string_cl(const char* s, size_t sz) {
    string_t* res = (string_t*)pvPortMalloc(sizeof(string_t));
    res->size = sz;
    res->alloc = sz + 1;
    res->p = (char*)pvPortMalloc(res->alloc);
    strncpy(res->p, s, res->alloc);
    res->p[sz] = 0;
    return res;
}

static string_t* new_string_cs(const string_t* s) {
    string_t* res = (string_t*)pvPortMalloc(sizeof(string_t));
    res->size = s->size;
    res->alloc = s->size + 1;
    res->p = (char*)pvPortMalloc(res->alloc);
    strncpy(res->p, s->p, res->alloc);
    return res;
}

static void string_reseve(string_t* s, size_t alloc) {
    if (s->alloc >= alloc) return; // already more or eq. than requested
    char* n_p = (char*)pvPortMalloc(alloc);
    strncpy(n_p, s->p, s->alloc);
    vPortFree(s->p);
    s->p = n_p;
    s->alloc = alloc;
}

static void string_push_back_c(string_t* s, const char c) {
    if (!c) return;
    if (s->size + 1 == s->alloc) {
        string_reseve(s, s->alloc + 16);
    }
    s->p[s->size] = c;
    s->p[++s->size] = 0;
}

static void string_push_back_cc(string_t* s, const char* cs) {
    if(!cs) return;
    size_t cs_sz = strlen(cs);
    if(!cs_sz) return;
    size_t sz = s->size + cs_sz;
    if (sz >= s->alloc) {
        string_reseve(s, sz + 1);
    }
    memcpy(s->p + s->size, cs, cs_sz);
    s->p[sz] = 0;
    s->size = sz;
}

static void string_push_back_cs(string_t* s, const string_t* cs) {
    if(!cs) return;
    size_t sz = s->size + cs->size;
    if (sz >= s->alloc) {
        string_reseve(s, sz + 1);
    }
    memcpy(s->p + s->size, cs->p, cs->size);
    s->p[sz] = 0;
    s->size = sz;
}

static void string_clip(string_t* s, size_t idx) {
    if (idx >= s->size) return;
    for (size_t i = idx; i <= s->size; ++i) {
        s->p[i] = s->p[i + 1];
    }
    if (s->size) --s->size;
}

static void string_resize(string_t* s, size_t sz) {
    if (sz >= s->alloc) {
        string_reseve(s, sz + 1);
    }
    s->p[sz] = 0;
    s->size = sz;
}

/// TODO: organize it
void* memset(void* p, int v, size_t sz);

static void string_insert_c(string_t* s, char c, size_t idx) {
    string_reseve(s, idx + 1);
    if (idx >= s->size) {
        size_t sps = idx - s->size;
        memset(s->p + s->size, ' ', sps);
        s->size += sps;
        string_push_back_c(s, c);
        return;
    }
    for (size_t i = s->size; i > idx; --i) {
        s->p[i] = s->p[i - 1];
    }
    s->p[idx] = c;
    s->p[++s->size] = 0;
}

static string_t* string_split_at(string_t* s, size_t idx) {
    if (idx >= s->size) {
        return new_string_v();
    }
    string_t* res = new_string_cc(s->p + idx);
    s->p[idx] = 0;
    s->size = idx;
    return res;
}

static void string_replace_cs(string_t* s, const char* cs) {
    size_t sz = strlen(cs);
    if (sz >= s->alloc) {
        string_reseve(s, sz + 1);
    }
    memcpy(s->p, cs, sz);
    s->p[sz] = 0;
    s->size = sz;
}

static void string_replace_ss(string_t* s, const string_t* ss) {
    size_t sz = ss->size;
    if (sz >= s->alloc) {
        string_reseve(s, sz + 1);
    }
    memcpy(s->p, ss->p, sz);
    s->p[sz] = 0;
    s->size = sz;
}

inline static const char* c_str(const string_t* s) {
    return s ? s->p : 0;
}

inline static size_t c_strlen(const string_t* s) {
    return s ? s->size : 0;
}

#ifdef __cplusplus
}
#endif

#endif

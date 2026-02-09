#ifndef M_OS_API_C_ARRAY_H
#define M_OS_API_C_ARRAY_H

void* pvPortMalloc(size_t sz); // 32
void vPortFree(void*); // 33

#define malloc(sz) pvPortMalloc(sz)
#define free(p) vPortFree(p)

// array of pointers to some objects
typedef void (*dealloc_fn_ptr_t)(void*);
typedef void* (*alloc_fn_ptr_t)(void);
typedef size_t (*size_fn_ptr_t)(void*);
typedef struct array {
    alloc_fn_ptr_t allocator;
    dealloc_fn_ptr_t deallocator;
    size_fn_ptr_t size_fn; // to ask each element
    void* *p; // pointer to array of pointers
    size_t alloc; // in bytes
    size_t size; // in elements
} array_t;

inline static array_t* new_array_v(alloc_fn_ptr_t allocator, dealloc_fn_ptr_t deallocator, size_fn_ptr_t size_fn) {
    array_t* res = (array_t*)malloc(sizeof(array_t));
    res->allocator = allocator ? allocator : (alloc_fn_ptr_t)pvPortMalloc;
    res->deallocator = deallocator ? deallocator : vPortFree;
    res->size_fn = size_fn;
    res->alloc = 10 * sizeof(void*);
    res->p = (void**)malloc(res->alloc);
    res->size = 0;
    return res;
}

inline static void delete_array(array_t* arr) {
    for (size_t i = 0; i < arr->size; ++i) {
        arr->deallocator(arr->p[i]);
    }
    free(arr->p);
    free(arr);
}

/// TODO: organize it
void* memcpy(void *__restrict dst, const void *__restrict src, size_t sz);

inline static int array_reserve(array_t* arr, size_t sz_bytes) {
    if (sz_bytes <= arr->alloc)
        return 0;

    void** p = (void**)malloc(sz_bytes);
    if (!p) return -1;
    if (arr->p && arr->size > 0) {
        memcpy(p, arr->p, arr->size * sizeof(void*));
        free(arr->p);
    }
    arr->p = p;
    arr->alloc = sz_bytes;
    return 0;
}

inline static size_t array_push_back(array_t* arr, void* data) {
    register size_t min_sz_bytes = (arr->size + 1) * sizeof(void*);
    if (min_sz_bytes > arr->alloc) {
        if (array_reserve(arr, min_sz_bytes) < 0) return 0;
    }
    arr->p[arr->size] = data;
    return arr->size++;
}

inline static void* array_get_at(array_t* arr, size_t n) { // by element number
    if (n >= arr->size) return 0;
    return arr->p[n];
}

inline static int array_resize(array_t* arr, size_t n) {
    if (n == arr->size) return 0;
    register size_t min_sz_bytes = n * sizeof(void*);
    if (min_sz_bytes > arr->alloc) {
        if (array_reserve(arr, min_sz_bytes) < 0) return -1;
    }
    if (n < arr->size) {
        for (size_t i = n; i < arr->size; ++i) {
            arr->deallocator(arr->p[i]);
        }
    } else {
        for (size_t i = arr->size; i < n; ++i) {
            arr->p[i] = arr->allocator();
        }
    }
    arr->size = n;
    return 0;
}

#endif /// M_OS_API_C_ARRAY_H

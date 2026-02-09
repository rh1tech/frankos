#ifndef M_API_C_HASH_i32
#define M_API_C_HASH_i32

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct hash_node_i32 {
    int key;
    void *value;
    struct hash_node_i32 *next;
} hash_node_i32_t;

typedef void (*dealloc_fn_ptr_t)(void*);
typedef struct {
    hash_node_i32_t **buckets;
    size_t bucket_count; // pow of 2 values (32, 64, 128)
    dealloc_fn_ptr_t dealloc;
} hash_table_i32_t;

static inline uint32_t hash_i32(int key) {
    uint32_t x = (uint32_t)key;
    // mix bits: from Thomas Wang integer hash
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

static inline hash_table_i32_t *hash_table_i32_create(size_t bucket_count, dealloc_fn_ptr_t dealloc) {
    hash_table_i32_t* t = (hash_table_i32_t*)pvPortMalloc(sizeof(hash_table_i32_t));
    if (!t) return NULL;
    t->buckets = (hash_node_i32_t**)pvPortCalloc(bucket_count, sizeof(hash_node_i32_t*));
    t->bucket_count = bucket_count;
    t->dealloc = dealloc;
    if (!t->buckets) {
        vPortFree(t);
        return NULL;
    }
    return t;
}

static inline void hash_table_i32_free(hash_table_i32_t *t) {
    if (!t) return;
    for (size_t i = 0; i < t->bucket_count; i++) {
        hash_node_i32_t *node = t->buckets[i];
        while (node) {
            hash_node_i32_t *tmp = node->next;
            if (t->dealloc && node->value) t->dealloc(node->value);
            vPortFree(node);
            node = tmp;
        }
    }
    vPortFree(t->buckets);
    vPortFree(t);
}

static inline void hash_table_i32_put(hash_table_i32_t *t, int key, void *value) {
    uint32_t h = hash_i32(key) & (t->bucket_count - 1);
    hash_node_i32_t* node = t->buckets[h];
    while (node) {
        if (node->key == key) { // replace existing
            if (t->dealloc && node->value && node->value != value) t->dealloc(node->value);
            node->value = value;
            return;
        }
        node = node->next;
    }
    node = (hash_node_i32_t*)pvPortMalloc(sizeof(*node));
    if (!node) return;
    node->key = key;
    node->value = value;
    node->next = t->buckets[h];
    t->buckets[h] = node;
}

static inline bool hash_table_i32_get(const hash_table_i32_t *t, int key, void **out_value) {
    uint32_t h = hash_i32(key) & (t->bucket_count - 1);
    hash_node_i32_t *node = t->buckets[h];
    while (node) {
        if (node->key == key) {
            if (out_value) *out_value = node->value;
            return true;
        }
        node = node->next;
    }
    return false;
}

#ifdef __cplusplus
}
#endif

#endif // M_API_C_HASH_i32

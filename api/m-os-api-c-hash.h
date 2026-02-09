#ifndef M_API_C_HASH
#define M_API_C_HASH

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct hash_node {
    char *key;
    uint32_t value;
    struct hash_node *next;
} hash_node_t;

typedef struct {
    hash_node_t **buckets;
    size_t bucket_count;
} hash_table_t;

// easy and fast FNV-1a (32-bit)
static uint32_t hash_str(const char *s) {
    uint32_t h = 2166136261u;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 16777619u;
    }
    return h;
}

inline static hash_table_t *hash_table_create(size_t bucket_count) {
    hash_table_t *t = pvPortMalloc(sizeof(*t));
    if (!t) return NULL;
    t->bucket_count = bucket_count;
    t->buckets = pvPortCalloc(bucket_count, sizeof(hash_node_t *));
    if (!t->buckets) {
        vPortFree(t);
        return NULL;
    }
    return t;
}

inline static void hash_table_free(hash_table_t *t) {
    if (!t) return;
    for (size_t i = 0; i < t->bucket_count; i++) {
        hash_node_t *node = t->buckets[i];
        while (node) {
            hash_node_t *tmp = node->next;
            vPortFree(node->key);
            vPortFree(node);
            node = tmp;
        }
    }
    vPortFree(t->buckets);
    vPortFree(t);
}

inline static char* pcStrDup(const char *s)
{
	size_t l = strlen(s);
	char *d = pvPortMalloc(l+1);
	if (!d) return NULL;
	return memcpy(d, s, l+1);
}

inline static bool hash_table_put(hash_table_t *t, const char *key, uint32_t value) {
    if (!t || !key) return 0;
    uint32_t h = hash_str(key) & (t->bucket_count - 1);
    hash_node_t *node = t->buckets[h];
    for (; node; node = node->next) {
        if (strcmp(node->key, key) == 0) {
             // replace
            node->value = value;
            return true;
        }
    }
    node = pvPortMalloc(sizeof(*node));
    if (!node) return 0;
    node->key = pcStrDup(key);
    if (!node->key) { // fault
        vPortFree(node);
        return false;
    }
    node->value = value;
    node->next = t->buckets[h];
    t->buckets[h] = node;
    return true;
}

inline static bool hash_table_get(const hash_table_t *t, const char *key, uint32_t *out_value) {
    if (!t || !key) return 0;
    uint32_t h = hash_str(key) & (t->bucket_count - 1);
    hash_node_t *node = t->buckets[h];
    for (; node; node = node->next) {
        if (strcmp(node->key, key) == 0) {
            if (out_value) *out_value = node->value;
            return true;
        }
    }
    return false; // not found
}

inline static int hash_table_remove(hash_table_t *t, const char *key) {
    if (!t || !key) return 0;
    uint32_t h = hash_str(key) % t->bucket_count;
    hash_node_t **pnode = &t->buckets[h];
    while (*pnode) {
        if (strcmp((*pnode)->key, key) == 0) {
            hash_node_t *tmp = *pnode;
            *pnode = tmp->next;
            vPortFree(tmp->key);
            vPortFree(tmp);
            return true;
        }
        pnode = &(*pnode)->next;
    }
    return false;
}

#ifdef __cplusplus
}
#endif

#endif // M_API_C_HASH

#include "Hashtable.h"

#include <string.h>
#include <stdio.h>

#define MAX_LOAD 0.8f
#define INIT_HT_SIZE 8

typedef struct _Hashtable {
    info* arr;
    uint size;
    size_t capacity;
    size_t key_size;
    int (*cmp_key)(void*, void*);
} _Hashtable;

Hashtable new_ht(size_t key_size, int (*cmp_key)(void*, void*)) {
    _Hashtable *ht = malloc(sizeof(*ht));
    if (ht == NULL) {
        return ht;
    }

    ht->size = 0;
    ht->capacity = INIT_HT_SIZE;
    ht->arr = calloc(ht->capacity, sizeof(*(ht->arr)));
    ht->key_size = key_size;
    ht->cmp_key = cmp_key;

    return ht;
}

void insert_ht(Hashtable ht, char *key, void *data) {
    if (ht->size == ht->capacity) {
        ht->capacity *= 2;
        ht->arr = realloc(ht->arr, ht->capacity);
    }

    info *entry = ht->arr + ht->size;
    entry->key = malloc(ht->key_size);
    memcpy(entry->key, key, ht->key_size);
    entry->data = data;
    ht->size++;
}

void* get_ht(Hashtable ht, char *key) {
    for (int i = 0; i < ht->size; i++) {
        void *it_key = ht->arr[i].key;
        if (it_key == NULL) {
            continue;
        }
        if (ht->cmp_key(key, it_key) == 0) {
            return ht->arr[i].data;
        }
    }

    return NULL;
}

void apply_ht(Hashtable ht, void (f)(void*)) {
    for (int i = 0; i < ht->size; i++) {
        info *inf = ht->arr + i;
        if (inf->key == NULL) {
            continue;
        }
        f(inf->data);
    }
}

void free_ht(Hashtable ht, void (*free_data)(void*)) {
    if (ht == NULL) {
        return;
    }

    for (int i = 0; i < ht->capacity; i++) {
        info *entry= ht->arr + i;
        free(entry->key);
        if (entry->data != NULL) {
            free_data(entry->data);
        }
    }

    free(ht->arr);
    free(ht);
}

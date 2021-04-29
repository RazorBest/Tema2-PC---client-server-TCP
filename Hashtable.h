#pragma once

#include <stdlib.h>
#include "Set.h"

typedef struct _Hashtable* Hashtable;

typedef struct info {
    void *key;
    void *data;
} info;

struct _Hashtable;

Hashtable new_ht(size_t key_size, int (*cmp_key)(void*, void*));

void insert_ht(Hashtable ht, char *key, void* data);
void* get_ht(Hashtable ht, char *key);
void apply_ht(Hashtable ht, void (f)(void*));

void free_ht(Hashtable ht, void (*free_data)(void*));


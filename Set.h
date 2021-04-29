#pragma once

#include "utils.h"

typedef struct client {
    char id[CLIENT_ID_LEN]; 
} client;

typedef struct Set Set;

typedef struct Set* SubscriptionSet;

SubscriptionSet new_set();

void insert_client(SubscriptionSet set, char *id);

#define FOR_EACH_IN_SET(set, id, instructions) \
    for (client *__client_it = _get_iterator(set); _has_next(set, __client_it)\
            ; __client_it = _next(set, __client_it)) { \
        char *(id) = __client_it->id; \
        (instructions); \
    } \

client* _get_iterator(Set *set);
int _has_next(Set *set, client *it);
client* _next(Set *set, client *it);

void free_set(SubscriptionSet set);


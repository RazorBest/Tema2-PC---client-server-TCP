#include <stdlib.h>
#include <string.h>
#include "Set.h"

typedef struct Set {
    client *clients;
    uint size;
} Set;

Set* new_set() {
    Set *set = malloc(sizeof(Set)); 

    set->size = 0;
    set->clients = malloc(100*sizeof(client));

    return set;
}

void insert_client(SubscriptionSet set, char *id) {
    client *cli = set->clients + set->size;
    memcpy(cli->id, id, CLIENT_ID_LEN);
}

client* _get_iterator(Set *s);
int _has_next(Set *s, client *it);
client* _next(Set *s, client *it);

void free_set(Set *set) {
    if (set == NULL) {
        return;
    }
    client *clients = set->clients;

    free(set);
    if (clients == NULL) {
        return;
    }

    free(clients);
}

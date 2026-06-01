#include <stdio.h>
#include "asset_hashmap.h"

unsigned int hash(char* key) {
    unsigned int h = 0;
    for (size_t i = 0; i < strlen(key); i++) {
        char c = key[i];
        h = (h * 31 + c); 
    }
    return (unsigned int) h;
}

AssetHashMap* new_asset_hashmap(int capacity) {
    AssetHashMapEntryList **buckets = calloc(capacity, sizeof(AssetHashMapEntryList *));

    AssetHashMap *h = malloc(sizeof(AssetHashMap));
    h->capacity = capacity;
    h->size = 0;
    h->buckets = buckets;

    return h;
}

void asset_hashmap_cleanup(AssetHashMap *map) {
    for (int i = 0; i < map->capacity; i++) {
        AssetHashMapEntryList *current = map->buckets[i];
        while (current != NULL) {
            AssetHashMapEntryList* next = current->next;
            free(current);
            current = next;
        }
    }
    free(map->buckets);
    free(map);
}

void asset_hashmap_resize(AssetHashMap *map) {
    int new_capacity = map->capacity * 2;
    AssetHashMapEntryList **new_buckets = calloc(new_capacity, sizeof(AssetHashMapEntryList *));

    // loop through each bucket
    for (int i = 0; i < map->capacity; i++) {
        AssetHashMapEntryList *current = map->buckets[i];

        // loop through every entry in the current bucket (linked list)
        while (current != NULL) {
            // save off the next entry before we overwrite it
            AssetHashMapEntryList *next_entry = current->next;
            // compute the new bucket index
            int new_index = hash(current->key) % new_capacity;

            // in case the bucket has entries, point our node at 
            // whatever is currently in there
            current->next = new_buckets[new_index];

            // make our `current` node the new head of the linked list
            new_buckets[new_index] = current;

            current = next_entry;
        }
    }

    free(map->buckets);

    map->buckets = new_buckets;
    map->capacity = new_capacity;
}

void asset_hashmap_put(AssetHashMap* map, char* key, char* value) {
    unsigned int index = hash(key) % map->capacity;

    AssetHashMapEntryList *head = map->buckets[index];

    AssetHashMapEntryList *current = head;
    while(current != NULL) {
        if (strcmp(current->key, key) == 0) {
            current->value = strdup(value);
            return;
        }
        current = current->next;
    } 

    AssetHashMapEntryList *new_entry = malloc(sizeof(AssetHashMapEntryList));
    new_entry->key = strdup(key);
    new_entry->value = strdup(value);
    new_entry->next = head;

    map->buckets[index] = new_entry;
    map->size += 1;

    // trigger resize if we are nearing capacity
    if ((float)map->size / map->capacity > 0.75f) {
        printf("hashmap has reached %d, at least 75 percent of its capacity. resizing\n", map->size);
        asset_hashmap_resize(map);
    }
}

char* asset_hashmap_get(AssetHashMap *map, char* key) {
    unsigned int index = hash(key) % map->capacity;

    AssetHashMapEntryList *current = map->buckets[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }

        current = current->next;
    }

    return NULL;
}

void asset_hashmap_delete(AssetHashMap *map, char* key) {
    unsigned int index = hash(key) % map->capacity;

    AssetHashMapEntryList *current = map->buckets[index];
    AssetHashMapEntryList *prev = NULL;

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (prev == NULL) {
                map->buckets[index] = current->next;
            } else {
                prev->next = current->next;
            }

            map->size -= 1;
            return;
        }

        prev = current;
        current = current->next;
    }
}
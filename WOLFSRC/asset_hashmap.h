typedef struct AssetHashMapEntryList {
    char* key;
    char* value;
    struct AssetHashMapEntryList* next;
} AssetHashMapEntryList;

typedef struct {
    AssetHashMapEntryList **buckets; // array of entries
    int capacity;   // size of buckets array
    int size; // number of key/value pairs stored
} AssetHashMap;

unsigned int hash(char* key);
AssetHashMap* new_asset_hashmap(int capacity);
void asset_hashmap_cleanup(AssetHashMap *map);
void asset_hashmap_resize(AssetHashMap *map);
void asset_hashmap_put(AssetHashMap* map, char* key, char* value);
char* asset_hashmap_get(AssetHashMap *map, char* key);
void asset_hashmap_delete(AssetHashMap *map, char* key);
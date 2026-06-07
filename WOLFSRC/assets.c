#include <stdio.h>
#include <stdlib.h>
#include "asset_hashmap.h"
#include "assets.h"

typedef struct {
    char* item;

} HashMapBucket;

typedef struct {
    int capacity;
    int size;
    HashMapBucket* buckets;
} HashMap;

typedef enum {
    ASSET_TEXTURE,
    ASSET_SPRITE,
    ASSET_FONT,
    ASSET_SOUND,
} AssetType;

typedef enum {
    ASSET_LOAD_OK,
    ASSET_LOAD_FILE_OPEN_ERR,
    ASSET_LOAD_FILE_READ_ERR,
    ASSET_LOAD_MEM_ERR
} LoadFileResult;

typedef struct {
    HashMap* fonts;
    HashMap* sounds;
    HashMap* textures;
    HashMap* sprites;
} AssetCache;


AssetCache* asset_cache = NULL; 

LoadFileResult load_binary_file(char* path, unsigned char* buffer) {
    FILE *file = fopen(path, "rb");
    if (!file) return ASSET_LOAD_FILE_OPEN_ERR;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    buffer = malloc(size);
    if (!buffer) {
        fclose(file);
        return ASSET_LOAD_MEM_ERR;
    }

    size_t bytes_read = fread(buffer, 1, size, file);
    if (bytes_read != size) {
        fclose(file);
        return ASSET_LOAD_FILE_READ_ERR;
    }

    fclose(file);
    return ASSET_LOAD_OK;
}


void LoadFonts(AssetHashMap* cache) {
    char* path = "assets/extracted/fonts";
    
    
}

/***
 * Load each asset into memory
 */
void AM_SetupAssets() {
    if (asset_cache != NULL) {
        // TODO: raise error, asset cache already initialized
    }

    asset_cache = malloc(sizeof(AssetCache));

    // TODO: adjust numbers below to match actual asset counts per type

    // Load fonts
    asset_cache->fonts = new_asset_hashmap(2);
    LoadFonts(asset_cache->fonts);

    // Load sounds
    asset_cache->sounds = new_asset_hashmap(100);

    // Load textures
    asset_cache->textures = new_asset_hashmap(100);

    // Load sprites
    asset_cache->sprites = new_asset_hashmap(100);
}

void AM_FreeAssets() {
    if (asset_cache == NULL) {
        // TODO: raise error, nothing to free
    }

    // Free each hashmap by calling its cleanup routine
    asset_hashmap_cleanup(asset_cache->fonts);
    asset_hashmap_cleanup(asset_cache->sounds);
    asset_hashmap_cleanup(asset_cache->textures);
    asset_hashmap_cleanup(asset_cache->sprites);

    // Free the cache itself
    free(asset_cache);
}

void* AM_GetGraphicsAsset(int asset_id) {
    // TODO: implement this
}

void* AM_GetAudioAsset(int asset_id) {
    // TODO: implement this
}

void* AM_GetMapAsset(int asset_id) {
    // TODO: implement this
}
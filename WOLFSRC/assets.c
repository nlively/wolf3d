#include <stdio.h>
#include <stdlib.h>
#include "asset_hashmap.h"
#include "assets.h"


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

MapAsset* AM_GetMapAsset(int asset_id) {
    // TODO: implement this
}

// The level Get/SetMapTile currently operate on. Set once per level load.
static MapAsset* current_map = NULL;

void AM_SetCurrentMap(int asset_id) {
    current_map = AM_GetMapAsset(asset_id);
}

uint16_t AM_GetMapTile(int plane, int x, int y) {
    // Out-of-range reads as empty; mirrors the always-solid map border the
    // original neighbor arithmetic relied on, without walking off the array.
    if (x < 0 || y < 0 || x >= MAP_DIM || y >= MAP_DIM)
        return 0;
    return current_map->planes[plane][y*MAP_DIM + x];
}

void AM_SetMapTile(int plane, int x, int y, uint16_t value) {
    if (x < 0 || y < 0 || x >= MAP_DIM || y >= MAP_DIM)
        return;
    current_map->planes[plane][y*MAP_DIM + x] = value;
}
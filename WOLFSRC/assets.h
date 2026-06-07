/* assets.h */
#ifndef ASSETS_H
#define ASSETS_H

#include <stdint.h>

#define MAP_PLANES	2		// 0 = walls/architecture, 1 = objects
#define MAP_DIM		64		// maps are 64*64


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


typedef struct {
	int			index;
	int			width, height;
	uint16_t	planes[MAP_PLANES][MAP_DIM*MAP_DIM];	// raw tile values, row-major
} MapAsset;

void     AM_SetupAssets();
void     AM_FreeAssets();

void     *AM_GetGraphicsAsset(int asset_id);
void     *AM_GetAudioAsset(int asset_id);

MapAsset *AM_GetMapAsset (int index_id);                 // whole map: metadata + bulk
void      AM_SetCurrentMap (int index_id);               // pick the live level — modern CA_CacheMap
uint16_t  AM_GetMapTile (int plane, int x, int y);       // read one tile of the live level
void      AM_SetMapTile (int plane, int x, int y, uint16_t value);   // write one tile

#endif
/* 
  this file owns the framebuffer that replaces the old 0xA000
  direct-to-hardware rendering
*/

#include "render.h"
#include <SDL2/SDL.h>

typedef struct {
    uint8_t framebuffer[SCREEN_W * SCREEN_H]; // back buffer: all drawing goes here
    uint8_t shown[SCREEN_W * SCREEN_H]; // last frame actually presented
    uint32_t palette[256]; // RGBA, set from VGA palette

    struct SDL_Window *window;
    struct SDL_Renderer *renderer;
    struct SDL_Texture *texture;
} VideoContext;

static VideoContext vid; // global instance, but not addressable outside this file

int R_Startup(const char *title, int scale) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return -1;
    vid.window = SDL_CreateWindow(
        title, 
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 
        SCREEN_W*scale, 
        SCREEN_H*scale, 
        0);
    if (!vid.window) {
        return -1;
    }
        
    // TODO: not sure if we need SDL_RENDERER_PRESENTVSYNC
    int flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    vid.renderer = SDL_CreateRenderer(vid.window, -1, flags);
    if (!vid.renderer) {
        return -1;
    }

    SDL_RenderSetLogicalSize(vid.renderer, SCREEN_W, SCREEN_H);
    vid.texture = SDL_CreateTexture(
        vid.renderer, 
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W,
        SCREEN_H);
    if (!vid.texture) {
        return -1;
    }
    
    // success
    return 0;
}

const char* R_GetError() {
    return SDL_GetError();
}

void R_Present(void) {
    void *pixels;
    int pitch;
    SDL_LockTexture(vid.texture, NULL, &pixels, &pitch);
    uint32_t *out = pixels;

    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        out[i] = vid.palette[vid.framebuffer[i]];
    }
    SDL_UnlockTexture(vid.texture);
    SDL_RenderClear(vid.renderer);
    SDL_RenderCopy(vid.renderer, vid.texture, NULL, NULL);
    SDL_RenderPresent(vid.renderer);

    memcpy(vid.shown, vid.framebuffer, sizeof vid.shown); // keep track of the previous frame shown
}

void R_Shutdown(void) {
    if (vid.texture) {
        SDL_DestroyTexture(vid.texture);
    }
    if (vid.renderer) {
        SDL_DestroyRenderer(vid.renderer);
    }
    if (vid.window) {
        SDL_DestroyWindow(vid.window);
    }
    SDL_Quit();
}

// TODO: update datatype of image_data
void R_DrawImage(int x, int y, void* image_data) {

}

void R_DrawPic(int x, int y, int chunknum)
{
    // do we need to figure picnum in order to determine width and height?  
    // probably not.  ideally we get that when we load the asset
	// int	picnum = chunknum - STARTPICS;
    R_DrawImage(x,y, AM_GetGraphicsAsset(chunknum));
}

void R_PutPixel(int x, int y, uint8_t color) {
    assert(x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H); // bounds check
    
    vid.framebuffer[y * SCREEN_W + x] = color;
}

void R_DrawColumn(int x, int y_top, int y_bottom, const uint8_t *texels) {
    // TODO: implement this later, call from WL_DRAW.C
}

void R_CaptureBackbuffer(uint8_t *dst) {
    memcpy(dst, vid.framebuffer, sizeof vid.framebuffer);
}

void R_RestoreShown(void) {
    memcpy(vid.framebuffer, vid.shown, sizeof vid.framebuffer);
}
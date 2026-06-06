/* 
  this file owns the framebuffer that replaces the old 0xA000
  direct-to-hardware rendering
*/

#include "render.h"
#include <SDL2/SDL.h>

VideoContext vid; // zero-initialized at startup

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
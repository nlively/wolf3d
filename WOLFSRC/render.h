/* render.h */
#ifndef RENDER_H
#define RENDER_H
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 200

typedef struct {
    uint8_t framebuffer[SCREEN_W * SCREEN_H]; // palette indices
    uint32_t palette[256]; // RGBA, set from VGA palette
    unsigned bufferofs; // in our modernization port, this will be a pixel offset, not a byte offset

    struct SDL_Window *window;
    struct SDL_Renderer *renderer;
    struct SDL_Texture *texture;
} VideoContext;

extern VideoContext vid; // global instance

int R_Startup(const char *title, int scale);
const char* R_GetError();
void R_Shutdown(void);
void R_Present(void);
void R_SetPalette(const uint8_t *vga_pal);
void R_DrawPic(int x, int y, int chunknum);
void R_DrawImage(int x, int y, void* image_data);

#endif
/* render.h */
#ifndef RENDER_H
#define RENDER_H
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 200

int R_Startup(const char *title, int scale);
const char* R_GetError();
void R_Shutdown(void);
void R_Present(void);
void R_SetPalette(const uint8_t *vga_pal);
void R_PutPixel(int x, int y, uint8_t color);
void R_CaptureBackbuffer(uint8_t *dst);
void R_RestoreShown(void);
void R_DrawColumn(int x, int y_top, int y_bottom, const uint8_t *texels);
void R_DrawPic(int x, int y, int chunknum);
void R_DrawImage(int x, int y, void* image_data);

#endif
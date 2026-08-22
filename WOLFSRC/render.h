/* render.h */
#ifndef RENDER_H
#define RENDER_H
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 200

#define FADED_IN 100
#define FADED_OUT 0

int R_Startup(const char *title, int scale);
const char* R_GetError();
void R_Shutdown(void);
void R_Present(void);
void R_SetPalette(const uint8_t *vga_pal);
uint32_t R_MapColor(uint8_t index);
void R_FillRect(int x, int y, int w, int h, uint32_t color);
void R_PutPixel(int x, int y, uint32_t color);
uint32_t R_GetPixel(int x, int y);
void R_CaptureBackbuffer(uint32_t *dst);
void R_RestoreShown(void);

int R_GetFadeLevel(void);
void R_SetFadeLevel(int fade_level);
void R_IncreaseFade(int amount);
void R_DecreaseFade(int amount);
bool R_IsScreenFadedOut(void);
bool R_IsScreenFadedIn(void);

void R_DrawColumn(int x, int y_top, int y_bottom, const uint32_t *texels);
void R_DrawPic(int x, int y, int chunknum);
void R_DrawImage(int x, int y, void* image_data);

#endif
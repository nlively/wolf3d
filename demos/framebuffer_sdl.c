#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>

#define SCREEN_W 320
#define SCREEN_H 200
#define WINDOW_SCALE 2

static uint32_t framebuffer[SCREEN_W * SCREEN_H];

static uint32_t make_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int rgba = (r << 24) | (g << 16) | (b << 8) | a;
}

static void put_pixel(int x, int y, uint32_t rgba) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    framebuffer[y * SCREEN_W + x] = rgba;
}

static void clear_screen(uint32_t rgba) {
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        framebuffer[i] = rgba;
    }
}

static void draw_test_pattern(int frame) {
    clear_screen(0x101020FF);

    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            uint8_t r = (uint8_t)((x + frame) % 256);
            uint8_t g = (uint8_t)((y + frame) % 256);
            uint8_t b = 120;
            put_pixel(x, y, make_rgba(r, g, b, 0xFF));
        }
    }
}

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "Framebuffer SDL2",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_W * WINDOW_SCALE,
        SCREEN_H * WINDOW_SCALE,
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W,
        SCREEN_H
    );

    bool running = true;
    SDL_Event event;
    int frame = 0;

    while (running) {
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = false;
        }

        draw_test_pattern(frame++);

        SDL_UpdateTexture(
            texture,
            NULL,
            framebuffer,
            SCREEN_W * sizeof(uint32_t)
        );

        SDL_RenderClear(renderer);

        SDL_Rect dst = {
            0, 0,
            SCREEN_W * WINDOW_SCALE,
            SCREEN_H * WINDOW_SCALE
        };

        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
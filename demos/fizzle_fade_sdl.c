#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 320
#define SCREEN_H 200
#define WINDOW_SCALE 2

static uint32_t framebuffer[SCREEN_W * SCREEN_H];

// the image we are revealing toward
static uint32_t target[SCREEN_W * SCREEN_H];

static uint32_t visible[SCREEN_W * SCREEN_H];

// randomized array of pixel indexes, used for pseudorandom fade effects.
// for performance, we'll build this once in main() and reuse it as needed.
static uint32_t *pixels;



static void put_pixel(uint32_t *fb, int x, int y, uint32_t rgba) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    fb[y * SCREEN_W + x] = rgba;
}

static void screen_paint(uint32_t *fb, uint32_t rgba) {
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            put_pixel(fb, x, y, rgba);
        }
    }
}

void fizzle_fade(SDL_Renderer *renderer, SDL_Texture *texture) {
    
    bool running = true;
    size_t reveal = 0;

    while (running) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return;
        }

        for (int i = 0; i < 1200 && reveal < SCREEN_W * SCREEN_H; i++) {
            uint32_t index = pixels[reveal++];
            visible[index] = target[index];
        }

        if (reveal == SCREEN_W * SCREEN_H) {
            running = false;
        }

        SDL_UpdateTexture(texture, NULL, visible, SCREEN_W * sizeof(uint32_t));

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }
}

void shuffle(uint32_t *array, size_t count) {
    for (size_t i = count - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);

        uint32_t tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    pixels = malloc(SCREEN_W * SCREEN_H * sizeof(uint32_t));
    // create an array of all pixel indexes
    for (uint32_t i = 0; i< SCREEN_W * SCREEN_H; i++) {
        pixels[i] = i;
    }

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

    shuffle(pixels, SCREEN_W * SCREEN_H);

    screen_paint(framebuffer, 0xFF0000FF);

    memcpy(visible, framebuffer, sizeof(visible));

    screen_paint(target, 0x000000FF);

    fizzle_fade(renderer, texture);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    free(pixels);

    return 0;
}
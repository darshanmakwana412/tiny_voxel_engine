#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <math.h>   /* add just after the other #includes */
#include <iostream>
#include <random>
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

#define return_defer(value) do { result = (value); goto defer; } while (0)

#define WIDTH 1000            /* pick any resolution you like */
#define HEIGHT 1000

static std::mt19937 rng(std::random_device{}());

struct Vec2 {

    float x, y;

    Vec2(float x, float y) : x(x), y(y) {}

    static inline
    Vec2 zero() {
        return Vec2(0.0f, 0.0f);
    }

    static inline
    Vec2 rand(float min = 0.0f, float max = 1.0f) {
        std::uniform_real_distribution<float> dist(min, max);
        return Vec2(dist(rng) * WIDTH, dist(rng) * HEIGHT);
    }

};

struct Color {
    
    uint8_t r, g, b, a;

    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : r(r), g(g), b(b), a(a) {}

    static inline
    Color red() {
        return Color(255, 0, 0, 255);
    }

    static inline
    Color black() {
        return Color(0, 0, 0, 255);
    }

    static inline
    Color rand() {
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        return Color(dist(rng), dist(rng), dist(rng), 255);
    }

    static inline
    Color from_uint32(uint32_t color) {
        return Color(
            (color >> 24) & 0xFF,  // Red
            (color >> 16) & 0xFF,  // Green
            (color >> 8) & 0xFF,   // Blue
            color & 0xFF           // Alpha
        );
    }

    inline
    uint32_t to_uint32() const {
        return (r << 24) | (g << 16) | (b << 8) | a;
    }

};

extern "C" inline
float min3(float a, float b, float c) {
    return std::min(a, std::min(b, c));
}

extern "C" inline
float max3(float a, float b, float c) {
    return std::max(a, std::max(b, c));
}

extern "C" inline
void draw_circle(
    uint32_t *pixels,
    Vec2 center,
    float radius,
    Color color
) {
    int x0 = std::max((int)(center.x - radius), 0);
    int y0 = std::max((int)(center.y - radius), 0);
    int x1 = std::min((int)(center.x + radius), WIDTH - 1);
    int y1 = std::min((int)(center.y + radius), HEIGHT - 1);

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float dx = x - center.x;
            float dy = y - center.y;
            if (dx * dx + dy * dy <= radius * radius) {
                pixels[y * WIDTH + x] = color.to_uint32();
            }
        }
    }
}

extern "C" inline
void draw_triangle (
    uint32_t *pixels,
    Vec2 p1, Vec2 p2, Vec2 p3,
    Color color
) {

    int minX = std::max(0, (int)min3(p1.x, p2.x, p3.x));
    int maxX = std::min(WIDTH - 1, (int)max3(p1.x, p2.x, p3.x));
    int minY = std::max(0, (int)min3(p1.y, p2.y, p3.y));
    int maxY = std::min(HEIGHT - 1, (int)max3(p1.y, p2.y, p3.y));

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float alpha = ((p2.y - p3.y) * (x - p3.x) + (p3.x - p2.x) * (y - p3.y)) /
                           ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));
            float beta  = ((p3.y - p1.y) * (x - p3.x) + (p1.x - p3.x) * (y - p3.y)) /
                           ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));
            float gamma = 1.0f - alpha - beta;

            if (alpha >= 0 && beta >= 0 && gamma >= 0) {
                pixels[y * WIDTH + x] = color.to_uint32();
            }
        }
    }
}

extern "C" inline
void render(
    uint32_t *pixels,
    float dt
) {

    // draw_circle(
    //     pixels,
    //     Vec2::rand(),
    //     5.0f,
    //     Color::rand()
    // );

    draw_triangle(
        pixels,
        Vec2::rand(),
        Vec2::rand(),
        Vec2::rand(),
        Color::rand()
    );

}

int main() {

    int result = 0;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;

    {

        if (SDL_Init(SDL_INIT_VIDEO) < 0) return_defer(1);

        window = SDL_CreateWindow(
            "Tiny Voxel Engine",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            WIDTH, HEIGHT,
            0
        );
        if (window == NULL) return_defer(1);

        renderer = SDL_CreateRenderer(
            window, -1,
            SDL_RENDERER_ACCELERATED |
            SDL_RENDERER_PRESENTVSYNC
        );
        if (renderer == NULL) return_defer(1);

        texture  = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            WIDTH, HEIGHT
        );
        if (texture == NULL) return_defer(1);

        Uint32 prev = SDL_GetTicks();
        while(true) {

            Uint32 curr = SDL_GetTicks();
            float dt = (curr - prev) / 1000.f;
            prev = curr;

            SDL_Event event;
            while (SDL_PollEvent(&event)) if (event.type == SDL_QUIT) return_defer(0);

            SDL_Rect whole = {0, 0, WIDTH, HEIGHT};
            void *pixels;
            int pitch;

            if (SDL_LockTexture(texture, &whole, (void **)&pixels, &pitch) < 0) return_defer(1);
            
            render((uint32_t *)pixels, dt);

            SDL_UnlockTexture(texture);

            if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0) < 0) return_defer(1);
            if (SDL_RenderClear(renderer) < 0) return_defer(1);
            if (SDL_RenderCopy(renderer, texture, &whole, &whole) < 0) return_defer(1);

            SDL_RenderPresent(renderer);

        }

    }

    defer:
    switch (result) {
        case 0:
            printf("OK\n");
            break;
        default:
            fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
            break;
    }

    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();

    return result;
}

// g++ sdl.cpp -lSDL2 -o main && ./main
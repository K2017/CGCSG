#include <iostream>
#include <glm/glm.hpp>
#include "SDLauxiliary.h"
#include "scene.h"
#include "examples.h"

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

constexpr int SCREEN_WIDTH = 720;
constexpr int SCREEN_HEIGHT = 720;
SDL_Surface *screen;
int t;


std::unique_ptr<Scene> scene;

std::vector<std::tuple<int, int, glm::vec3>> framebuffer;


// ----------------------------------------------------------------------------
// FUNCTIONS

void Update();

void Draw();

int main(int argc, char *argv[]) {
    screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
    t = SDL_GetTicks();    // Set start value for timer.

    framebuffer = std::vector<std::tuple<int, int, vec3>>(SCREEN_WIDTH * SCREEN_HEIGHT);

    scene = example::triangles(SCREEN_WIDTH, SCREEN_HEIGHT);
    //scene->setDebugProperties(DebugProperties{.normals = true});

    Draw();
    Update();

    SDL_SaveBMP(screen, "screenshot.bmp");
    return 0;
}

void Update() {
    // Compute frame time:
    int t2 = SDL_GetTicks();
    auto dt = float(t2 - t);
    t = t2;
    std::cout << "Render time: " << dt << " ms." << std::endl;
    auto keystate = SDL_GetKeyState(nullptr);

    auto camera = scene->getActiveCamera();
    if (keystate[SDLK_UP]) {
        camera->translate(0, 0, -0.1f);
    }
    if (keystate[SDLK_DOWN]) {
        camera->translate(0, 0, 0.1f);
    }
    if (keystate[SDLK_LEFT]) {
        camera->rotate(vec3{0, 1, 0}, -3);
    }
    if (keystate[SDLK_RIGHT]) {
        camera->rotate(vec3{0, 1, 0}, 3);
    }

    auto light = scene->getLight(0);
    if (light)
    {
        // light: forward backward z
        if (keystate[SDLK_w]) {
            light->position += vec3{0, 0, 0.1f};
        }
        if (keystate[SDLK_s]) {
            light->position += vec3{0, 0, -0.1f};
        }

        // light: left right x
        if (keystate[SDLK_a]) {
            light->position += vec3{-0.1f, 0, 0};
        }
        if (keystate[SDLK_d]) {
            light->position += vec3{0.1f, 0, 0};
        }

        // light: up down y
        if (keystate[SDLK_q]) {
            light->position += vec3{0, 0.1f, 0};
        }
        if (keystate[SDLK_e]) {
            light->position += vec3{0, -0.1f, 0};
        }
    }
}

void SetFramebuffer(int x, int y, const glm::vec3 &color) {
    framebuffer.at(y * screen->pitch / 4 + x) = std::make_tuple(x, y, color);
}

void Draw() {
#pragma omp parallel for
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
#pragma omp parallel for
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            auto ray = Ray::fromView(x, y, SCREEN_WIDTH, SCREEN_HEIGHT, scene->getActiveCamera());

            auto color = scene->trace(ray);
            SetFramebuffer(x, y, color);
        }
    }

    if (SDL_MUSTLOCK(screen))
        SDL_LockSurface(screen);

    for (auto &[x, y, color] : framebuffer) {
        PutPixelSDL(screen, x, y, color);
    }

    if (SDL_MUSTLOCK(screen))
        SDL_UnlockSurface(screen);

    SDL_UpdateRect(screen, 0, 0, 0, 0);
}


#include <iostream>
#include <glm/glm.hpp>
#include "sdf/sdf.h"
#include "SDLauxiliary.h"
#include "scene.h"

using namespace std;
using glm::vec3;
using glm::mat3;
using namespace sdf;
using namespace sdf::ops;

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH = 720;
const int SCREEN_HEIGHT = 720;
SDL_Surface *screen;
int t;


std::unique_ptr<Scene> scene;

std::vector<std::tuple<int, int, vec3>> framebuffer;


// ----------------------------------------------------------------------------
// FUNCTIONS

void Update();

void Draw();

int main(int argc, char *argv[]) {
    screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
    t = SDL_GetTicks();    // Set start value for timer.

    framebuffer = std::vector<std::tuple<int, int, vec3>>(SCREEN_WIDTH * SCREEN_HEIGHT);

    scene = std::make_unique<Scene>(SceneProperties{
            .backgroundColor{0.8, 0.8, 0.9},
            .illumination = true,
            .fresnel = true,
            .shadowing = true,
            .shadowIntensity = 16.f,
            .maxDepth = 8
    });

    auto mainLight = std::make_shared<Light>(vec3{-0.4, -1.0, -0.7}, vec3{1, 1, 1}, 10.f);
    scene->addLight(mainLight);

    auto camera = std::make_shared<Camera>(vec3{0, 0, -3.f}, vec3{0, 1.f, 0}, (float) SCREEN_WIDTH);
    scene->setActiveCamera(camera);

    auto sphere1 = std::make_shared<Sphere>(0.5f);
    sphere1->setMaterial(Material{
            .albedo{0.1, 0.75, 0.1},
            .ks = 1.0f,
            .p = 36.f,
            .ior = 1.5f,
            .transmittance = 1.0f,
    });
    auto bubble = Builder<Onion>(sphere1, 0.01).build();
    auto trBubble = Builder<Transform>(bubble, vec3{0, 0.48f, 0}).build();
    scene->addSDFObject(trBubble);

    auto sphere3 = Builder<Sphere>(0.8f).build();
    sphere3->setMaterial(Material{
            .albedo{.15, .75, .75},
            .ks = 1.0,
            .ior = 1.3,
            .transmittance = 0.
    });
    auto trSphere3 = Builder<Transform>(sphere3, vec3{-1., 0, 0.6}).build();
    scene->addSDFObject(trSphere3);

    auto torus = Builder<Torus>(glm::vec2{0.5, 0.1})
            .withMaterial(Material{
                    .albedo{0.75, 0.1, 0.1},
                    .ks = 1.f,
                    .p = 36.f,
                    .ior = 1.45f,
                    .transmittance = 0.85f,
            }).build();
    auto trTorus = std::make_shared<Transform>(torus, vec3{0.25, 0, 0.1}, vec3{45, 0, 0});

    auto glob = std::make_shared<Union>(trBubble, trTorus, true, 0.08f);
    scene->addSDFObject(glob);

    auto ground = std::make_shared<sdf::Plane>(vec3{0, -1.f, 0}, 1.f);
    ground->setMaterial(Material{
            .albedo{0.75f, 0.75f, 0.75f},
            .ks = 0.1f,
            .p = 128,
            .ior = 1.33f
    });
    scene->addSDFObject(ground);

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
    cout << "Render time: " << dt << " ms." << endl;
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

void SetFramebuffer(int x, int y, const vec3 &color) {
    framebuffer.at(y * screen->pitch / 4 + x) = std::make_tuple(x, y, color);
}

void Draw() {

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


//
// Created by Yannis on 28/05/2021.
//

#ifndef PROJECT_EXAMPLES_H
#define PROJECT_EXAMPLES_H

#include "scene.h"
#include "sdf/sdf.h"

namespace example {
    using glm::vec3;
    using glm::mat3;
    using namespace sdf;
    using namespace sdf::ops;

    typedef std::unique_ptr<Scene> ScenePtr;

    // A scene with a single sphere, rendered with surface normals as color.
    ScenePtr normals(int width, int height) {
        auto scene = std::make_unique<Scene>(SceneProperties{
                .backgroundColor{0.2, 0.2, 0.25},
                .debugNormals = true
        });

        auto mainLight = std::make_shared<Light>(vec3{-0.4, -1.0, -0.7}, vec3{1, 1, 1}, 10.f);
        scene->addLight(mainLight);

        auto camera = std::make_shared<Camera>(vec3{0, 0, -3.f}, vec3{0, 1.f, 0}, (float) width);
        scene->setActiveCamera(camera);

        auto sphere = std::make_shared<Sphere>(0.5f);
        scene->addSDFObject(sphere);

        return scene;
    }

    // A scene with a single unlit sphere, highlighting proper intersection detection.
    ScenePtr raymarch(int width, int height) {
        auto scene = std::make_unique<Scene>();

        auto camera = std::make_shared<Camera>(vec3{0, 0, -3.f}, vec3{0, 1.f, 0}, (float) width);
        scene->setActiveCamera(camera);

        auto sphere = std::make_shared<Sphere>(0.5f);
        sphere->setMaterial(Material::Default());
        scene->addSDFObject(sphere);

        return scene;
    }

    // A phong shaded sphere.
    ScenePtr phong(int width, int height) {
        auto scene = std::make_unique<Scene>(SceneProperties {
            .illumination = true
        });

        auto mainLight = std::make_shared<Light>(vec3{-0.4, -1.0, -0.7}, vec3{1, 1, 1}, 10.f);
        scene->addLight(mainLight);

        auto camera = std::make_shared<Camera>(vec3{0, 0, -3.f}, vec3{0, 1.f, 0}, (float) width);
        scene->setActiveCamera(camera);

        auto sphere = std::make_shared<Sphere>(0.5f);
        sphere->setMaterial(Material {
            .albedo = {0.8, 0.8, 0.8},
            .ks = 1.0,
            .p = 36
        });
        scene->addSDFObject(sphere);

        return scene;
    }

    ScenePtr scene1(int width, int height) {
        auto scene = std::make_unique<Scene>(SceneProperties{
                .backgroundColor{0.8, 0.8, 0.9},
                .illumination = true,
                .fresnel = true,
                .shadowing = true,
                .shadowIntensity = 16.f,
                .maxDepth = 8
        });

        auto mainLight = std::make_shared<Light>(vec3{-0.4, -1.0, -0.7}, vec3{1, 1, 1}, 10.f);
        scene->addLight(mainLight);

        auto camera = std::make_shared<Camera>(vec3{0, 0, -3.f}, vec3{0, 1.f, 0}, (float) width);
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

        return scene;
    }
}
#endif //PROJECT_EXAMPLES_H

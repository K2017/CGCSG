#ifndef PROJECT_EXAMPLES_H
#define PROJECT_EXAMPLES_H

#include "scene.h"
#include "sdf/sdf.h"

namespace example {
    using glm::vec3;
    using glm::mat3;
    using namespace sdf;
    using namespace sdf::ops;
    using namespace sdf::utils;

    using ScenePtr = std::unique_ptr<Scene>;

    // A scene with a single sphere, rendered with surface normals as color.
    ScenePtr sphereNormals(int width, int height) {
        auto scene = std::make_unique<Scene>(SceneProperties{
                .backgroundColor{0.2, 0.2, 0.25},
        });
        scene->setDebugProperties({
                                          .normals = true
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
    ScenePtr sphereRaymarching(int width, int height) {
        auto scene = std::make_unique<Scene>();

        auto camera = std::make_shared<Camera>(vec3{0, 0, -3.f}, vec3{0, 1.f, 0}, (float) width);
        scene->setActiveCamera(camera);

        auto sphere = std::make_shared<Sphere>(0.5f);
        sphere->setMaterial(Material::Default());
        scene->addSDFObject(sphere);

        return scene;
    }

    // A phong shaded sphere.
    ScenePtr spherePhong(int width, int height) {
        auto scene = std::make_unique<Scene>(SceneProperties{
                .illumination = true
        });

        auto mainLight = std::make_shared<Light>(vec3{-0.4, -1.0, -0.7}, vec3{1, 1, 1}, 10.f);
        scene->addLight(mainLight);

        auto camera = std::make_shared<Camera>(vec3{0, 0, -3.f}, vec3{0, 1.f, 0}, (float) width);
        scene->setActiveCamera(camera);

        auto sphere = std::make_shared<Sphere>(0.5f);
        sphere->setMaterial(Material{
                .albedo = {0.8, 0.8, 0.8},
                .ks = 1.0,
                .p = 36
        });
        scene->addSDFObject(sphere);

        return scene;
    }

    // Complex scene with a single hollow die (dice) object constructed with CSG, smoothly combined with a ring
    ScenePtr hollowDieCSG(int width, int height) {
        auto scene = std::make_unique<Scene>(SceneProperties{
                .backgroundColor{0.8, 0.8, 0.9},
                .illumination = true,
                .fresnel = true,
                //.shadowing = true,
                .maxDepth = 8
        });

        auto mainLight = std::make_shared<Light>(vec3{-0.4, -1.0, -0.7}, vec3{1, 1, 1}, 10.f);
        scene->addLight(mainLight);

        auto light1 = std::make_shared<Light>(vec3{1.3, 0.5, -1.1}, vec3{0.4, 0.4, 1}, 15.f);
        scene->addLight(light1);

        auto camera = std::make_shared<Camera>(vec3{0, 0, -3.f}, vec3{0, 1.f, 0}, (float) width);
        scene->setActiveCamera(camera);

        Material bodyMat = {
                .albedo{0.2, 0.5, 0.2},
                .ks = 1,
                .p = 128,
                .ior = 1.52,
                .transmittance = 0.8,
                .absorption = 0.5
        };

        Material dotMat = {
                .albedo{1, 1, 1},
                .ks = 0.1,
                .p = 36,
        };

        Material ringMat = {
                .albedo{0.75, 0.1, 0.1},
                .ks = 1.f,
                .p = 36.f,
                .ior = 1.45f,
                .transmittance = 0.8,
        };

        auto box = Builder<Box>(vec3{0.5, 0.5, 0.5}).withMaterial(bodyMat).asNode() %= 0.02;
        auto body = (box | Builder<Sphere>(0.75).withMaterial(bodyMat).asNode() % 0.02) ^ 0.04;

        std::vector<vec3> holePositions = {
                // One
                {0,     -0.51, 0},
                // Two
                {0.51,  -0.25, 0.25},
                {0.51,  0.25,  -0.25},
                // Three
                {0,     0,     -0.51},
                {-0.25, -0.25, -0.51},
                {0.25,  0.25,  -0.51},
                // Four
                {0.25,  0.25,  0.51},
                {0.25,  -0.25, 0.51},
                {-0.25, 0.25,  0.51},
                {-0.25, -0.25, 0.51},
                // Five
                {-0.51, 0.25,  0.25},
                {-0.51, 0.25,  -0.25},
                {-0.51, 0,     0},
                {-0.51, -0.25, 0.25},
                {-0.51, -0.25, -0.25},
                // Six
                {0.25,  0.51,  0.25},
                {-0.25, 0.51,  0.25},
                {0.25,  0.51,  -0.25},
                {-0.25, 0.51,  -0.25},
                {0.25,  0.51,  0},
                {-0.25, 0.51,  0}
        };

        std::shared_ptr<Node> dots = make_empty();
        for (auto pos : holePositions) {
            auto s = Builder<Sphere>(0.1).withMaterial(dotMat).withTransform(pos).asNode();
            dots = dots + s;
        }

        auto die = Builder<Transform>(
                body - dots % 0.01,
                vec3{0, 0.25, 0},
                vec3{std::numbers::pi / 6, std::numbers::pi / 4, 0}
        ).asNode();

        auto ring = Builder<Torus>(glm::vec2{0.5, 0.1})
                .withMaterial(ringMat)
                .withTransform(
                        vec3{0.5, -0.5, -0.2},
                        vec3{std::numbers::pi / 1.5, std::numbers::pi / 6, 0}
                ).asNode();
        scene->addSDFObject(ring + (die) % 0.1);

        auto ground = Builder<Plane>(vec3{0, -1.f, 0}, 1.f).asNode();
        ground->setMaterial(Material{
                .albedo{0.8, 0.8, 0.8},
                .ks = 0.2f,
                .p = 128,
                .ior = 1.33f
        });
        scene->addSDFObject(ground);

        return scene;
    }

    ScenePtr triangles(int height, int width) {
        auto scene = std::make_unique<Scene>(SceneProperties{
                .backgroundColor{0.8, 0.8, 0.9},
                .illumination = true,
                .fresnel = true,
                .maxDepth = 8
        });

        auto mainLight = std::make_shared<Light>(vec3{-0.4, -1.0, -0.7}, vec3{1, 1, 1}, 10.f);
        scene->addLight(mainLight);

        auto light1 = std::make_shared<Light>(vec3{1.3, 0.5, -1.1}, vec3{0.4, 0.4, 1}, 15.f);
        scene->addLight(light1);

        auto camera = std::make_shared<Camera>(vec3{0, 0, -3.f}, vec3{0, 1.f, 0}, (float) width);
        scene->setActiveCamera(camera);

        Material mat1 = {
                .albedo{0.75, 0.75, 0.1},
                .ks = 1.f,
                .p = 36.f,
                .ior = 1.45f,
                .transmittance = 0.8,
        };

        Material mat2 = {
                .albedo{0.1, 0.75, 0.75},
                .ks = 1.f,
                .p = 36.f,
                .ior = 1.45f,
                .transmittance = 0.8,
        };

        auto triangle1 = Builder<Triangle>(vec3{0, 0, 0}, vec3{0.8, 0, 0}, vec3{0.8, -0.8, 0})
                .withMaterial(mat1).asNode();
        auto triangle2 = Builder<Triangle>(vec3{0, -0.8, 0}, vec3{0.6, -0.2, 0.8}, vec3{0.6, -0.2, -0.8})
                .withMaterial(mat2).asNode();

        auto positioned = Builder<Transform>(triangle1 + triangle2 % 0.2,
                                             vec3{-0.4, 0.3, 0},
                                             vec3{std::numbers::pi / 6, 0, 0},
                                             vec3{2}).asNode();

        scene->addSDFObject(positioned);

        auto ground = Builder<Plane>(vec3{0, -1.f, 0}, 1.f).asNode();
        ground->setMaterial(Material{
                .albedo{0.8, 0.8, 0.8},
                .ks = 0.2f,
                .p = 128,
                .ior = 1.33f
        });
        scene->addSDFObject(ground);

        return scene;
    }
}
#endif //PROJECT_EXAMPLES_H

#ifndef SECONDLAB_SCENE_H
#define SECONDLAB_SCENE_H

#include <glm/glm.hpp>

#include "ray.h"
#include "light.h"
#include "sdf/sdf.h"
#include <numbers>

struct SceneProperties {
    vec3 backgroundColor = vec3{0};
    bool illumination = false;
    bool fresnel = false;
    bool shadowing = false;
    bool absorption = false;
    float shadowIntensity = 16.f;
    int maxRaymarchSteps = 500;
    float maxRaymarchDist = 20.f;
    int maxDepth = 4;
};

struct DebugProperties {
    bool normals = false;
    bool depth = false;
};

class Scene {
public:
    explicit Scene(const SceneProperties &properties = SceneProperties{}) : scene(properties) {}

    vec3 trace(const Ray &ray);

    void addLight(const std::shared_ptr<Light> &light) {
        lights.push_back(light);
    }

    void addCamera(const std::shared_ptr<Camera> &camera) {
        cameras.push_back(camera);
    }

    void setActiveCamera(const std::shared_ptr<Camera> &camera) {
        auto it = std::find(cameras.begin(), cameras.end(), camera);
        if (it != cameras.end()) {
            activeCamIndex = it - cameras.begin();
        } else {
            // camera not in scene
            addCamera(camera);
            activeCamIndex = 0;
        }
    }

    std::shared_ptr<Camera> getActiveCamera() {
        if (!cameras.empty()) {
            return cameras[activeCamIndex];
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<Light> getLight(int index) {
        if (index >= lights.size()) {
            return nullptr;
        }
        return lights.at(index);
    }

    void addSDFObject(const std::shared_ptr<sdf::Node> &sdf) {
        sdfNodes.push_back(sdf);
    }

    void setDebugProperties(const DebugProperties& properties) {
        debug = properties;
    }

private:
    SceneProperties scene;
    DebugProperties debug;

    std::vector<std::shared_ptr<sdf::Node>> sdfNodes;
    std::vector<std::shared_ptr<Light>> lights;
    std::vector<std::shared_ptr<Camera>> cameras;
    int activeCamIndex = 0;

    std::pair<vec3, vec3> computeLightingModel(const vec3 &p, const vec3 &N, const vec3 &V, const Material &material);

    std::pair<std::shared_ptr<sdf::Node>, float> raycast(const Ray &ray);
    std::pair<std::shared_ptr<sdf::Node>, float> minimumSurface(const vec3 &p);

    float computeShadow(const Ray &r, float k);

    float computeFresnel(const vec3 &I, const vec3 &N, float etai, float etat = 1);

    vec3
    finalColor(const Material &material, const vec3 &diffuse, const vec3 &specular, const vec3 &refraction,
               const vec3 &reflection,
               float kr);

    void addDefaultLight() {
        auto light = std::make_shared<Light>(vec3{0, -1.0, -0.5}, vec3{1, 1, 1}, 10.f);
        addLight(light);
    }

    vec3 trace(const Ray &ray, int depth);
};

// Phong lighting model.
std::pair<vec3, vec3>
Scene::computeLightingModel(const vec3 &p, const vec3 &N, const vec3 &V, const Material &material) {
    vec3 I_D{0, 0, 0}, I_S{0, 0, 0};

    vec3 sBias = N * 0.1f;

    for (auto &light : lights) {
        const vec3 L = glm::normalize(light->position - p);
        const vec3 R = glm::normalize(glm::reflect(-L, N));

        float dotLN = glm::dot(L, N);
        float dotRV = glm::dot(R, V);

        vec3 D = light->color * glm::max(dotLN, 0.0f) * light->intensity / (float)(4.0f * std::numbers::pi * glm::length(light->position - p));
        vec3 S = light->color * glm::pow(glm::max(dotRV, 0.0f), material.p) * light->intensity;

        if (scene.shadowing) {
            const vec3 pos = p + sBias;

            const Ray r(pos, L);
            float shadowFactor = computeShadow(r, scene.shadowIntensity);

            D *= shadowFactor;
            S *= shadowFactor;
        }

        I_D += D;
        I_S += S;
    }

    return {I_D, I_S};
}

vec3 Scene::trace(const Ray &ray, int depth) {

    auto[node, t] = raycast(ray);

    if (t < 0) {
        return scene.backgroundColor;
    }

    vec3 p = ray.at(t);

    auto sample = node->sampleAt(p);
    vec3 N = node->normal(p, 1e-4f);

    bool inside = glm::dot(N, -ray.dir) < 0;
    vec3 facingNormal = inside ? -N : N;

    vec3 diffuse{0}, specular{0};
    Material material = sample.material;

    if (debug.normals) {
        return N * 0.5f + 0.5f;
    }

    if (debug.depth) {
        vec3 c = p - getActiveCamera()->pos();
        vec3 col = vec3{1.0f / c.z};
        return col;
    }

    if (scene.illumination) {
        std::tie(diffuse, specular) = computeLightingModel(p, facingNormal, -ray.dir, material);
    } else {
        diffuse = vec3{1};
    }

    float kr = 0.5f;
    vec3 refraction{0}, reflection{0}, absorption{0, 0, 0};
    if (scene.fresnel && depth > 0) {
        vec3 R = glm::normalize(glm::reflect(ray.dir, facingNormal));

        float etai = 1;
        float etat = material.ior;

        if (inside) {
            std::swap(etai, etat);
        }

        vec3 T = glm::normalize(glm::refract(ray.dir, facingNormal, etai / etat));

        kr = computeFresnel(ray.dir, facingNormal, etai, etat);

        // reflection
        if (material.ks > 0) {
            vec3 bias = facingNormal * 1e-4f;
            vec3 rlPos = p + bias;

            reflection = trace(Ray(rlPos, R), depth - 1);
        }

        // refraction
        if (kr < 1 && material.transmittance > 0 && material.ks > 0) {
            vec3 bias = facingNormal * 1e-4f;
            vec3 rfPos = p - bias;

            Ray transmitted(rfPos, T);

            if (scene.absorption) {
                auto[inner, dist] = raycast(transmitted);
                absorption = material.albedo * (1.0f / glm::pow((dist + 1), 1.f)) * material.absorption;
            }

            refraction = trace(transmitted, depth - 1);
        }
    }

    vec3 final = finalColor(material, diffuse, specular, refraction, reflection, kr);
    return final;
}

vec3 Scene::finalColor(const Material &material, const vec3 &diffuse, const vec3 &specular, const vec3 &refraction,
                       const vec3 &reflection, float kr) {
    vec3 rl = reflection * kr * material.ks;
    vec3 rf = refraction * (1 - kr) * material.transmittance;
    vec3 fresnel = rl + rf;

    vec3 I_A = material.albedo * (material.ka / (float) lights.size());
    vec3 I_D = diffuse * material.albedo * material.kd;
    vec3 I_S = specular * kr * material.ks;

    vec3 local = I_A + I_D + I_S;
    return glm::clamp(local + fresnel, 0.f, 1.f);
}

// Find the Node that produces the smallest signed distance out of all nodes.
std::pair<std::shared_ptr<sdf::Node>, float> Scene::minimumSurface(const vec3 &p) {

    float min = std::numeric_limits<float>::infinity();
    std::shared_ptr<sdf::Node> minNode;
    for (auto &node : sdfNodes) {
        float d = node->signedDistance(p);
        if (d < min) {
            min = d;
            minNode = node;
        }
    }
    return std::make_pair(minNode, min);
}

// Implementation of Sphere Casting, adapted for negative distances.
std::pair<std::shared_ptr<sdf::Node>, float> Scene::raycast(const Ray &ray) {
    float t = 0.0f;

    std::shared_ptr<sdf::Node> hit;
    for (int i = 0; i < scene.maxRaymarchSteps; ++i) {
        float min = std::numeric_limits<float>::infinity();
        std::tie(hit, min) = minimumSurface(ray.at(t));
        min = glm::abs(min);
        if (min < 10e-6) {
            break;
        }
        t += min;
        if (t > scene.maxRaymarchDist) {
            t = -1;
            break;
        }
    }
    return std::make_pair(hit, t);
}

float Scene::computeFresnel(const vec3 &I, const vec3 &N, float etai, float etat) {
    float kr;

    float cTheta = glm::clamp(glm::dot(N, I), -1.f, 1.f);

    //if (cTheta > 0) { std::swap(etai, etat); }

    float sPhi = etai / etat * glm::sqrt(glm::max(1 - cTheta * cTheta, 0.f));

    if (sPhi >= 1) {
        kr = 1;
    } else {
        float cPhi = glm::sqrt(glm::max(1 - sPhi * sPhi, 0.f));
        cTheta = glm::abs(cTheta);
        float Rs = ((etat * cTheta) - (etai * cPhi)) / ((etat * cTheta) + (etai * cPhi));
        float Rp = ((etai * cTheta) - (etat * cPhi)) / ((etai * cTheta) + (etat * cPhi));
        kr = (Rs * Rs + Rp * Rp) / 2.0f;
    }

    return kr;
}

// Soft shadows for SDFs. https://iquilezles.org/www/articles/rmshadows/rmshadows.htm
float Scene::computeShadow(const Ray &r, float k) {
    float res = 1.0f;
    float ph = std::numeric_limits<float>::max();
    float t = 0.0f;
    for (int i = 0; i < scene.maxRaymarchSteps; ++i) {
        vec3 p = r.at(t);
        auto[closest, h] = minimumSurface(p);

        if (h < 0.001) {
            return 0.0f;
        }

        float y = h * h / (2.0f * ph);
        float d = glm::sqrt(glm::abs(h * h - y * y));
        res = glm::min(res, k * d / glm::max(0.0001f, t - y));
        ph = h;
        t += h;
        if (t > scene.maxRaymarchDist) {
            break;
        }
    }
    return res;
}

vec3 Scene::trace(const Ray &ray) {
    if (lights.empty()) {
        addDefaultLight();
    }
    return trace(ray, scene.maxDepth);
}


#endif //SECONDLAB_SCENE_H

//
// Created by Yannis on 06/04/2021.
//

#ifndef SECONDLAB_SCENE_H
#define SECONDLAB_SCENE_H

#include <glm/glm.hpp>

#include "ray.h"
#include "light.h"
#include "sdf/sdf.h"
#include <numbers>

struct SceneProperties {
    bool illumination = false;
    bool fresnel = false;
    bool shadowing = false;
    float shadowIntensity = 1.f;
    int maxRaymarchSteps = 500;
    float maxRaymarchDist = 20.f;
    int maxDepth = 0;
};

class Scene {
public:
    explicit Scene(const vec3 &background, const SceneProperties &properties = SceneProperties{});

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
        return lights.at(index);
    }

    void addSDFObject(const std::shared_ptr<sdf::Node> &sdf) {
        sdfNodes.push_back(sdf);
    }

    std::pair<std::shared_ptr<sdf::Node>, float> minimumSurface(const vec3 &p);

private:
    std::vector<std::shared_ptr<sdf::Node>> sdfNodes;
    vec3 background;
    std::vector<std::shared_ptr<Light>> lights;
    std::vector<std::shared_ptr<Camera>> cameras;
    int activeCamIndex = 0;
    float shadowBias = 0.1f;

    std::pair<vec3, vec3> computeLightingModel(const vec3 &p, const vec3 &N, const vec3 &V, const Material &material);

    std::pair<std::shared_ptr<sdf::Node>, float> raycast(const Ray &ray);

    float computeShadow(const Ray &r, float k);

    static float computeFresnel(const vec3 &I, const vec3 &N, float etai, float etat = 1);

    SceneProperties properties;

    vec3
    finalColor(const Material &material, const vec3 &diffuse, const vec3 &specular, const vec3 &refraction,
               const vec3 &reflection,
               float kr);

    vec3 trace(const Ray &ray, int depth);
};

Scene::Scene(const vec3 &background, const SceneProperties &properties)
        :
        background(background),
        properties(properties) {}

std::pair<vec3, vec3>
Scene::computeLightingModel(const vec3 &p, const vec3 &N, const vec3 &V, const Material &material) {
    vec3 I_D{0, 0, 0}, I_S{0, 0, 0};

    vec3 sBias = N * 0.1f;

    for (auto &light : lights) {
        const vec3 L = glm::normalize(light->position - p);
        const vec3 R = glm::normalize(glm::reflect(-L, N));

        float dotLN = glm::dot(L, N);
        float dotRV = glm::dot(R, V);

        I_D += light->color * glm::max(dotLN, 0.0f) * light->intensity /
               (float) (4.0f * std::numbers::pi * glm::length(light->position - p));
        I_S += light->color * glm::pow(glm::max(dotRV, 0.0f), material.p) * light->intensity;
        if (properties.shadowing) {
            const vec3 pos = p + sBias;

            const Ray r(pos, L);
            float factor = computeShadow(r, properties.shadowIntensity);
            I_D *= factor;
            I_S *= factor;

        }
    }

    return {I_D, I_S};
}

vec3 Scene::trace(const Ray &ray, int depth) {

    auto [node, t] = raycast(ray);

    if (t < 0) {
        return background;
    }

    vec3 p = ray.at(t);

    auto sample = node->sampleAt(p);
    vec3 N = node->normal(p, 1e-5f);

    bool inside = glm::dot(N, -ray.dir) < 0;
    vec3 facingNormal = inside ? -N : N;

    vec3 diffuse{0}, specular{0};
    Material material = sample.material;

    if (properties.illumination) {
        std::tie(diffuse, specular) = computeLightingModel(p, facingNormal, -ray.dir, material);
    }

    float kr = 0.5f;
    vec3 refraction{0}, reflection{0};
    if (properties.fresnel && depth > 0) {
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
            vec3 bias = facingNormal * 1e-3f;
            vec3 rlPos = p + bias;

            reflection = trace(Ray(rlPos, R), depth - 1);
        }

        // refraction
        if (kr < 1 && material.transmittance > 0 && material.ks > 0) {
            vec3 bias = facingNormal * 1e-3f;
            vec3 rfPos = p - bias;

            refraction = trace(Ray(rfPos, T), depth - 1);
        }
    }

    vec3 final = finalColor(material, diffuse, specular, refraction, reflection, kr);
    return final;
}

vec3 Scene::finalColor(const Material &material, const vec3 &diffuse, const vec3 &specular, const vec3 &refraction,
                       const vec3 &reflection, float kr) {
    kr = (1 - material.transmittance) + kr * (material.transmittance);
    vec3 rl = reflection * kr * material.ks;
    vec3 rf = refraction * (1 - kr);
    vec3 fresnel = rl + rf;

    vec3 I_A = material.albedo * (material.ka / (float) lights.size());
    vec3 I_D = diffuse * material.albedo * material.kd;
    vec3 I_S = specular * kr * material.ks;

    vec3 local = I_A + I_D + I_S;
    return glm::clamp(local + fresnel, 0.f, 1.f);
}

std::pair<std::shared_ptr<sdf::Node>, float> Scene::minimumSurface(const vec3 &p) {

    float min = std::numeric_limits<float>::infinity();
    std::shared_ptr<sdf::Node> minNode;
    for (auto& node : sdfNodes) {
        float d = node->signedDistance(p);
        if (d < min) {
            min = d;
            minNode = node;
        }
    }
    return std::make_pair(minNode, min);
}

std::pair<std::shared_ptr<sdf::Node>, float> Scene::raycast(const Ray &ray) {
    float t = 0.0f;

    std::shared_ptr<sdf::Node> hit;
    for (int i = 0; i < properties.maxRaymarchSteps; ++i) {
        float min = std::numeric_limits<float>::infinity();
        std::tie(hit, min) = minimumSurface(ray.at(t));
        min = glm::abs(min);
        if (min < 10e-6) {
            break;
        }
        t += min;
        if (t > properties.maxRaymarchDist) {
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

float Scene::computeShadow(const Ray &r, float k) {
    float res = 1.0f;
    float ph = std::numeric_limits<float>::max();
    float t = 0.0f;
    for (int i = 0; i < properties.maxRaymarchSteps; ++i) {
        vec3 p = r.at(t);
        auto [closest, h] = minimumSurface(p);

        if (h < 0.001) {
            return 0.0f;
        }

        float y = h * h / (2.0f * ph);
        float d = glm::sqrt(h * h - y * y);
        res = glm::min(res, k * d / glm::max(0.0f, t - y));
        ph = h;
        t += h;
        if (t > properties.maxRaymarchDist) {
            break;
        }
    }
    return res;
}

vec3 Scene::trace(const Ray &ray) {
    return trace(ray, properties.maxDepth);
}


#endif //SECONDLAB_SCENE_H

//
// Created by Yannis on 14/05/2021.
//

#ifndef PROJECT_MATERIAL_H
#define PROJECT_MATERIAL_H

#include <glm/glm.hpp>

class Material {
public:

    glm::vec3 albedo;
    float kd = 1;
    float ka = 0.1f;
    float ks = 0;
    float p = 4.0f;
    float ior = 1;
    float transmittance = 0.0f;

    static Material mix(const Material& a, const Material& b, float factor) {
        Material mixed;
        mixed.albedo = glm::mix(a.albedo, b.albedo, factor);
        mixed.kd = glm::mix(a.kd, b.kd, factor);
        mixed.ka = glm::mix(a.ka, b.ka, factor);
        mixed.ks = glm::mix(a.ks, b.ks, factor);
        mixed.p = glm::mix(a.p, b.p, factor);
        mixed.ior = glm::mix(a.ior, b.ior, factor);
        mixed.transmittance = glm::mix(a.transmittance, b.transmittance, factor);
        return mixed;
    }

    static Material Default() {
        return {
            .albedo{0.8, 0.8, 0.8},
            .kd = 0.8
        };
    }
};

#endif //PROJECT_MATERIAL_H

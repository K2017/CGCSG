//
// Created by Yannis on 22/05/2021.
//

#ifndef PROJECT_COMMON_H
#define PROJECT_COMMON_H

#include <glm/glm.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include "../material.h"

namespace sdf {
    struct Sample {
        float value = std::numeric_limits<float>::infinity();
        Material material;
    };

    class Node {
    protected:
        Material mat;

    public:

        Node() {
            outside = Material{
                    .ks = 1.0f,
                    .ior = 1.0f,
                    .transmittance = 0.0f,
            };
        }

        [[nodiscard]] virtual Sample sampleAt(const glm::vec3 &p) = 0;

        [[nodiscard]] virtual float signedDistance(const glm::vec3 &p) = 0;

        [[nodiscard]] glm::vec3 normal(const glm::vec3 &p, float e) {
            const glm::vec2 k = glm::vec2(1.f, -1.f) * 0.5773f;
            glm::vec3 a = glm::xyy(k);
            glm::vec3 b = glm::yyx(k);
            glm::vec3 c = glm::yxy(k);
            glm::vec3 d = glm::xxx(k);
            glm::vec3 val = a * sampleAt(p + a * e).value
                            + b * sampleAt(p + b * e).value
                            + c * sampleAt(p + c * e).value
                            + d * sampleAt(p + d * e).value;
            return glm::normalize(val);
        }

        void setMaterial(const Material &material) {
            mat = material;
        }

        [[nodiscard]] virtual Material getMaterial(const glm::vec3& p) const {
            return mat;
        }

        Material outside;
    };
}

#endif //PROJECT_COMMON_H

#ifndef PROJECT_COMMON_H
#define PROJECT_COMMON_H

#include <glm/glm.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include "../material.h"

namespace sdf {

    /* Represents the compound return value of a SDF. Includes the sampled distance and material. */
    struct Sample {
        float value = std::numeric_limits<float>::infinity();
        Material material;
    };

    /* Base class for all SDF objects. Used for building CSG trees. */
    class Node {
    public:

        Node() = default;

        /* Obtain a sample of the SDF at the given point, containing distance and material. */
        [[nodiscard]] virtual Sample sampleAt(const glm::vec3 &p) = 0;

        /* Evaluate the SDF at a given point, yielding a distance value. */
        [[nodiscard]] virtual float signedDistance(const glm::vec3 &p) = 0;

        /**
         * Compute the normal vector at a given point.
         * @details
         * The point need not be on the surface of the SDF, in which case the normal represents the tangent vector to
         * the gradient of the field represented by the SDF. May be costly to compute depending on the complexity of the SDF.
         * @param p Point to evaluate
         * @param e tolerance
         * @return
         */
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

        static std::shared_ptr<Node> Empty;
    };

    /* Empty node for building CSG trees. */
    class Empty final : public Node {
    public:
        Sample sampleAt(const vec3 &p) override {
            return {};
        }

        float signedDistance(const vec3 &p) override {
            return std::numeric_limits<float>::infinity();
        }
    };
}

#endif //PROJECT_COMMON_H

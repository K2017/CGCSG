#ifndef PROJECT_SHAPES_H
#define PROJECT_SHAPES_H

#include <glm/glm.hpp>

namespace sdf {
    using glm::vec3;
    using glm::vec2;

    // Base class for SDF shapes. This is where implementations of signed distance functions reside.
    class Primitive : public Node {
    protected:
        Material mat;
    public:
        Primitive() = default;

        Sample sampleAt(const glm::vec3 &p) final {
            Sample sample;
            sample.value = signedDistance(p);
            sample.material = mat;
            return sample;
        }

        void setMaterial(const Material &material) {
            mat = material;
        }

        [[nodiscard]] Material getMaterial(const vec3 &p) const {
            return mat;
        }
    };

    // Sphere SDF. Defined by a radius.
    class Sphere : public Primitive {
    private:
        float r;
    public:
        explicit Sphere(float radius) : r(radius) {}

        float signedDistance(const glm::vec3 &p) override {
            return glm::length(p) - r;
        }
    };

    // Plane SDF. Defined by a normal vector and a height.
    class Plane : public Primitive {
    private:
        const vec3 normal;
        float h;
    public:
        Plane(const vec3 &n, float h) : h(h), normal(n) {}

        float signedDistance(const glm::vec3 &p) override {
            return glm::dot(p, normal) + h;
        }
    };

    // Torus SDF. Defined by inner and outer radii.
    class Torus : public Primitive {
    private:
        vec2 r;
    public:
        explicit Torus(const vec2 &radii) : r(radii) {}

        float signedDistance(const glm::vec3 &p) override {
            vec2 q = vec2(glm::length(glm::xz(p)) - r.x, p.y);
            return glm::length(q) - r.y;
        }
    };

    // Closed Box SDF. Defined by extent from origin.
    class Box : public Primitive {
    private:
        glm::vec3 dimensions;
    public:
        explicit Box(const glm::vec3& dimensions) : dimensions(dimensions) {}

        float signedDistance(const glm::vec3 &p) override {
            glm::vec3 q = glm::abs(p) - dimensions;
            return glm::length(glm::max(q, 0.0f)) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f);
        }
    };

    // Triangle SDF. Defined by three vertices in world space.
    class Triangle : public Primitive {
    private:
        glm::vec3 v0, v1, v2;
        glm::vec3 e0, e1, e2;
        glm::vec3 normal;

        glm::vec3 c0, c1, c2;
        float l0, l1, l2, ln;

    public:
        Triangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2)
                : v0(v0), v1(v1), v2(v2) {
            e0 = v1 - v0;
            e1 = v2 - v1;
            e2 = v0 - v2;
            normal = glm::normalize(glm::cross(e0, e2));

            c0 = glm::cross(e0, normal);
            c1 = glm::cross(e1, normal);
            c2 = glm::cross(e2, normal);

            l0 = 1.0f / glm::dot(e0, e0);
            l1 = 1.0f / glm::dot(e1, e1);
            l2 = 1.0f / glm::dot(e2, e2);
            ln = 1.0f / glm::dot(normal, normal);
        }

        float signedDistance(const vec3 &p) override {
            vec3 p0 = p - v0;
            vec3 p1 = p - v1;
            vec3 p2 = p - v2;

            float val;
            if ((glm::sign(glm::dot(c0, p0)) +
                 glm::sign(glm::dot(c1, p1)) +
                 glm::sign(glm::dot(c2, p2))) < 2.0) {
                val = glm::min(
                        glm::min(
                                glm::length(e0 * glm::clamp(glm::dot(e0, p0) * l0, 0.0f, 1.0f) - p0),
                                glm::length(e1 * glm::clamp(glm::dot(e1, p1) * l1, 0.0f, 1.0f) - p1)
                        ),
                        glm::length(e2 * glm::clamp(glm::dot(e2, p2) * l2, 0.0f, 1.0f) - p2)
                );

            } else {
                val = glm::dot(normal, p0) * glm::dot(normal, p0) * ln;
                val = glm::sqrt(val);
            }

            return val - 0.001f;
        }
    };
}

#endif //PROJECT_SHAPES_H

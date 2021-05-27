//
// Created by Yannis on 22/05/2021.
//

#ifndef PROJECT_OPS_H
#define PROJECT_OPS_H

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sdf::ops {

    using glm::vec3;
    using glm::vec4;
    using glm::mat4;

    Sample min(const Sample &a, const Sample &b) {
        if (a.value < b.value) {
            return a;
        }
        return b;
    }

    Sample max(const Sample &a, const Sample &b) {
        if (a.value >= b.value) {
            return a;
        }
        return b;
    }

    class Union final : public Node {
    private:
        const std::shared_ptr<Node> a, b;
        bool smooth;
        float k;
    public:
        Union(std::shared_ptr<Node> a, std::shared_ptr<Node> b, bool smooth = false, float k = 0.1f)
                : a(std::move(a)), b(std::move(b)), smooth(smooth), k(k) {
        }

        Sample sampleAt(const glm::vec3 &p) override {
            Sample s1 = a->sampleAt(p);
            Sample s2 = b->sampleAt(p);

            if (smooth) {
                Sample sample;
                float d1 = s1.value;
                float d2 = s2.value;
                float h = glm::clamp(0.5f + 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
                sample.value = glm::mix(d2, d1, h) - k * h * (1.0f - h);
                sample.material = Material::mix(b->getMaterial(p), a->getMaterial(p), h);
                return sample;
            }
            return min(s1, s2);
        }

        Material getMaterial(const glm::vec3 &p) const override {
            float d1 = a->signedDistance(p);
            float d2 = b->signedDistance(p);

            if (smooth) {
                float h = glm::clamp(0.5f + 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
                return Material::mix(b->getMaterial(p), a->getMaterial(p), h);
            }
            if (d1 < d2) {
                return a->getMaterial(p);
            }
            return b->getMaterial(p);
        }

        float signedDistance(const glm::vec3 &p) override {
            float d1 = a->signedDistance(p);
            float d2 = b->signedDistance(p);

            if (smooth) {
                float h = glm::clamp(0.5f + 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
                return glm::mix(d2, d1, h) - k * h * (1.0f - h);
            }

            return glm::min(d1, d2);
        }
    };

    class Subtraction final : public Node {
    private:
        const std::shared_ptr<Node> a, b;
        bool smooth;
        float k;
    public:
        Subtraction(std::shared_ptr<Node> a, std::shared_ptr<Node> b, bool smooth = false, float k = 1)
                : a(std::move(a)), b(std::move(b)), smooth(smooth), k(k) {

        }

        [[nodiscard]] Sample sampleAt(const glm::vec3 &p) override {

            Sample sample;
            Sample s1 = b->sampleAt(p);
            Sample s2 = a->sampleAt(p);
            if (smooth) {
                float d1 = s1.value;
                float d2 = s2.value;
                float h = glm::clamp(0.5f - 0.5f * (d2 + d1) / k, 0.0f, 1.0f);
                sample.value = glm::mix(d2, -d1, h) + k * h * (1.0f - h);
            } else {
                if (-s1.value > s2.value) {
                    sample.value = -s1.value;
                } else {
                    sample.value = s2.value;
                }
            }
            sample.material = s2.material;
            return sample;
        }

        [[nodiscard]] float signedDistance(const glm::vec3 &p) override {
            float d1 = b->signedDistance(p);
            float d2 = a->signedDistance(p);
            if (smooth) {
                float h = glm::clamp(0.5f - 0.5f * (d2 + d1) / k, 0.0f, 1.0f);
                return glm::mix(d2, -d1, h) + k * h * (1.0f - h);
            } else {
                return glm::max(-d1, d2);
            }
        }
    };

    class Intersection final : public Node {
    private:
        const std::shared_ptr<Node> a, b;
        bool smooth;
        float k;
    public:
        Intersection(std::shared_ptr<Node> a, std::shared_ptr<Node> b, bool smooth = false, float k = 1) : a(
                std::move(a)), b(std::move(b)), smooth(smooth), k(k) {}

        [[nodiscard]] Sample sampleAt(const glm::vec3 &p) override {
            return max(a->sampleAt(p), b->sampleAt(p));
        }

        [[nodiscard]] float signedDistance(const glm::vec3 &p) override {
            return glm::max(a->signedDistance(p), b->signedDistance(p));
        }
    };

    class Transform final : public Node {

        const std::shared_ptr<Node> node;
        mat4 transform;
        vec3 scale;
    public:
        explicit Transform(std::shared_ptr<Node> node, const vec3 &translate = vec3(0, 0, 0),
                           const vec3 &rotate = vec3(0, 0, 0), const vec3 &scale = vec3(1, 1, 1))
                : scale(scale), node(std::move(node)), transform(1) {
            transform = glm::translate(transform, translate);
            vec2 axis(1, 0);
            glm::quat q = glm::angleAxis(rotate.x, glm::xyy(axis));
            q *= glm::angleAxis(rotate.y, glm::yxy(axis));
            q *= glm::angleAxis(rotate.z, glm::yyx(axis));
            transform *= glm::mat4_cast(q);
        }

        Sample sampleAt(const glm::vec3 &p) override {
            Sample sample;
            sample.value = signedDistance(p);
            sample.material = sample.value <= 0.01 ? node->getMaterial(p) : outside;
            return sample;
        }

        Material getMaterial(const glm::vec3 &p) const override {
            return node->getMaterial(p);
        }

        [[nodiscard]] float signedDistance(const glm::vec3 &p) override {
            glm::vec3 pp = vec3((glm::inverse(transform) * vec4(p, 1)));
            pp /= scale;
            float d = node->signedDistance(pp);
            return d / glm::min(scale.x, glm::min(scale.y, scale.z));
        }
    };

    class Elongate final : public Node {
        std::shared_ptr<Node> node{};
        glm::vec3 amount;
    public:
        explicit Elongate(std::shared_ptr<Node> node, const glm::vec3 &amount) : amount(amount),
                                                                                 node(std::move(node)) {}

        [[nodiscard]] Sample sampleAt(const glm::vec3 &p) override {
            glm::vec3 q = glm::abs(p) - amount;
            Sample sample = node->sampleAt(glm::sign(p) * glm::max(q, 0.0f));
            sample.value += glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f);
            return sample;
        }

        [[nodiscard]] float signedDistance(const glm::vec3 &p) override {
            glm::vec3 q = glm::abs(p) - amount;
            float d = node->signedDistance(glm::sign(p) * glm::max(q, 0.0f));
            return d + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f);
        }
    };

    class Round final : public Node {
        std::shared_ptr<Node> node{};
        float radius;
    public:
        explicit Round(std::shared_ptr<Node> node, float radius) : node(std::move(node)), radius(radius) {}

        [[nodiscard]] Sample sampleAt(const glm::vec3 &p) override {
            Sample sample = node->sampleAt(p);
            sample.value -= radius;
            return sample;
        }

        [[nodiscard]] float signedDistance(const glm::vec3 &p) override {
            float d = node->signedDistance(p);
            return d - radius;
        }
    };

    class Onion final : public Node {
        std::shared_ptr<Node> node{};
        float thickness;
    public:
        explicit Onion(std::shared_ptr<Node> node, float radius) : node(std::move(node)), thickness(radius) {}

        Sample sampleAt(const glm::vec3 &p) override {
            Sample sample;
            sample.value = signedDistance(p);
            sample.material = sample.value <= 0.01 ? node->getMaterial(p) : outside;
            return sample;
        }

        Material getMaterial(const glm::vec3 &p) const override {
            return node->getMaterial(p);
        }

        [[nodiscard]] float signedDistance(const glm::vec3 &p) override {
            float d = node->signedDistance(p);
            return glm::abs(d) - thickness;
        }
    };
}

#endif //PROJECT_OPS_H

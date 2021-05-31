#ifndef PROJECT_OPS_H
#define PROJECT_OPS_H

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sdf::ops {

    using glm::vec3;
    using glm::vec4;
    using glm::mat4;

    // Base class for Unary operations on Signed Distance Functions
    class UnaryOp : public Node {
    protected:
        const std::shared_ptr<Node> node;
    public:
        explicit UnaryOp(std::shared_ptr<Node> node) : node(std::move(node)) {};

        [[nodiscard]] std::shared_ptr<Node> getChild() const {
            return node;
        }
    };

    // Base class for Binary operations on Signed Distance Functions
    class BinaryOp : public Node {
    protected:
        const std::shared_ptr<Node> a, b;
    public:
        BinaryOp(std::shared_ptr<Node> a, std::shared_ptr<Node> b) : a(std::move(a)), b(std::move(b)) {}

        [[nodiscard]] std::shared_ptr<Node> getLeftChild() const {
            return a;
        }

        [[nodiscard]] std::shared_ptr<Node> getRightChild() const {
            return b;
        }
    };

    // Smooth minimum function with mix factor. As described here: https://iquilezles.org/www/articles/smin/smin.htm
    std::pair<float, float> sminN(float a, float b, float k, float n) {
        float h = glm::max(k - abs(a - b), 0.0f) / k;
        float m = glm::pow(h, n) * 0.5f;
        float s = m * k / n;
        return (a < b) ? std::make_pair(a - s, m) : std::make_pair(b - s, m - 1.0f);
    }

    class Union final : public BinaryOp {
    private:
        bool smooth;
        float k;
    public:
        Union(std::shared_ptr<Node> a, std::shared_ptr<Node> b, bool smooth = false, float k = 0.1f)
                : BinaryOp(std::move(a), std::move(b)), smooth(smooth), k(k) {}

        Sample sampleAt(const glm::vec3 &p) override {
            Sample s1 = a->sampleAt(p);
            Sample s2 = b->sampleAt(p);

            float d1 = s1.value;
            float d2 = s2.value;

            if (smooth) {
                Sample sample;

                auto[s, m] = sminN(d1, d2, k, 3);

                float h = glm::clamp(0.5f + 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
                sample.value = s;
                sample.material = Material::mix(s2.material, s1.material, h);
                return sample;
            }
            if (d1 < d2) {
                return s1;
            } else {
                return s2;
            }
        }

        float signedDistance(const glm::vec3 &p) override {
            float d1 = a->signedDistance(p);
            float d2 = b->signedDistance(p);

            if (smooth) {
                auto[s, m] = sminN(d1, d2, k, 3);
                return s;
            }

            return glm::min(d1, d2);
        }
    };

    class Difference final : public BinaryOp {
    private:
        bool smooth;
        float k;
    public:
        Difference(std::shared_ptr<Node> a, std::shared_ptr<Node> b, bool smooth = false, float k = 1)
                : BinaryOp(std::move(a), std::move(b)), smooth(smooth), k(k) {}

        [[nodiscard]] Sample sampleAt(const glm::vec3 &p) override {
            Sample sample;

            Sample s1 = b->sampleAt(p);
            Sample s2 = a->sampleAt(p);

            float d1 = s1.value;
            float d2 = s2.value;

            if (smooth) {
                float h = glm::clamp(0.5f - 0.5f * (d2 + d1) / k, 0.0f, 1.0f);
                sample.value = glm::mix(d2, -d1, h) + k * h * (1.0f - h);
                sample.material = Material::mix(s2.material, s1.material, h);
            } else {
                if (-d1 > d2) {
                    sample.value = -d1;
                    sample.material = s1.material;
                } else {
                    sample.value = d2;
                    sample.material = s1.material;
                }
            }
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

    class Intersection final : public BinaryOp {
    private:
        bool smooth;
        float k;
    public:
        Intersection(std::shared_ptr<Node> a, std::shared_ptr<Node> b, bool smooth = false, float k = 1)
                : BinaryOp(std::move(a), std::move(b)), smooth(smooth), k(k) {}

        [[nodiscard]] Sample sampleAt(const glm::vec3 &p) override {
            Sample sample;

            Sample s1 = b->sampleAt(p);
            Sample s2 = a->sampleAt(p);

            float d1 = s1.value;
            float d2 = s2.value;

            if (smooth) {
                float h = glm::clamp(0.5f - 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
                sample.value = glm::mix(d2, d1, h) + k * h * (1.0f - h);
                sample.material = Material::mix(s2.material, s1.material, h);
            } else {
                if (d1 > d2) {
                    sample = s1;
                } else {
                    sample = s2;
                }
            }

            return sample;
        }

        [[nodiscard]] float signedDistance(const glm::vec3 &p) override {
            float d1 = b->signedDistance(p);
            float d2 = a->signedDistance(p);

            if (smooth) {
                float h = glm::clamp(0.5f - 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
                return glm::mix(d2, d1, h) + k * h * (1.0f - h);
            }
            return glm::max(d1, d2);
        }
    };

    class Transform final : public UnaryOp {
    public:
        explicit Transform(std::shared_ptr<Node> node,
                           const vec3 &translate = vec3(0, 0, 0),
                           const vec3 &rotate = vec3(0, 0, 0),
                           const vec3 &scale = vec3(1, 1, 1))
                : UnaryOp(std::move(node)), scale(scale), transform(1) {
            // Translation
            transform = glm::translate(transform, translate);

            // Rotation
            vec2 axis(1, 0);
            glm::quat q = glm::angleAxis(rotate.x, glm::xyy(axis));
            q *= glm::angleAxis(rotate.y, glm::yxy(axis));
            q *= glm::angleAxis(rotate.z, glm::yyx(axis));
            transform *= glm::mat4_cast(q);

            // Scale performed separately
        }

        Sample sampleAt(const glm::vec3 &p) override {
            auto t = transformPoint(p);
            Sample sample = node->sampleAt(t);
            sample.value = correctDistance(sample.value);
            return sample;
        }

        [[nodiscard]] float signedDistance(const glm::vec3 &p) override {
            auto t = transformPoint(p);
            float d = node->signedDistance(t);
            return correctDistance(d);
        }

    private:
        mat4 transform;
        vec3 scale;

        [[nodiscard]] vec3 transformPoint(const vec3 &point) const {
            glm::vec3 transformed = vec3((glm::inverse(transform) * vec4(point / scale, 1)));
            return transformed;
        }

        [[nodiscard]] float correctDistance(float d) const {
            return d / glm::min(scale.x, glm::min(scale.y, scale.z));
        }
    };

    class Elongate final : public UnaryOp {
    private:
        glm::vec3 amount;
    public:
        explicit Elongate(std::shared_ptr<Node> node, const glm::vec3 &amount) : UnaryOp(std::move(node)),
                                                                                 amount(amount) {}

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

    class Round final : public UnaryOp {
    private:
        float radius;
    public:
        explicit Round(std::shared_ptr<Node> node, float radius) : UnaryOp(std::move(node)), radius(radius) {}

        [[nodiscard]] Sample sampleAt(const glm::vec3 &p) override {
            Sample sample = node->sampleAt(p);
            sample.value -= radius;
            return sample;
        }

        [[nodiscard]] float signedDistance(const glm::vec3 &p) override {
            float d = node->signedDistance(p);
            return d - radius;
        }

        [[nodiscard]] float getRadius() const {
            return radius;
        }
    };

    class Onion final : public UnaryOp {
        float thickness;
    public:
        explicit Onion(std::shared_ptr<Node> node, float thickness) : UnaryOp(std::move(node)), thickness(thickness) {}

        Sample sampleAt(const glm::vec3 &p) override {
            Sample sample = node->sampleAt(p);
            sample.value = signedDistance(p);
            return sample;
        }

        [[nodiscard]] float signedDistance(const glm::vec3 &p) override {
            float d = node->signedDistance(p);
            return glm::abs(d) - thickness;
        }
    };
}

#endif //PROJECT_OPS_H

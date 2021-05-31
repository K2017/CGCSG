#ifndef PROJECT_UTILS_H
#define PROJECT_UTILS_H

/***
 * Various utility methods for constructing CSG trees out of SDFs
 */
namespace sdf::utils {

    /**
     * A utility class for constructing SDF Nodes
     * @tparam T Type of SDF to construct, must be derived from sdf::Node
     */
    template<class T> requires std::is_base_of_v<Node, T>
    class Builder {
        using Transform = sdf::ops::Transform;
        using NodePtr = std::shared_ptr<T>;
    private:
        NodePtr node;
    public:
        template<typename ...Args>
        requires requires(T t, Args &&... args) { T(args...); }
        explicit Builder(Args &&... args) {
            node = std::make_shared<T>(std::forward<Args>(args)...);
        }

        NodePtr asNode() {
            return node;
        }

        Builder<T> withMaterial(const Material &material) requires std::is_base_of_v<Primitive, T> {
            node->setMaterial(material);
            return *this;
        }

        Builder<Transform> withTransform(const vec3 &position = vec3{0},
                                         const vec3 &rotation = vec3{0},
                                         const vec3 &scale = vec3{1}) {
            return Builder<Transform>(node, position, rotation, scale);
        }
    };

    // Shorthand for making and empty CSG Node
    static std::shared_ptr<Node> make_empty() {
        return std::make_shared<Empty>();
    }

    /**
     * Obtain the Union of two CSG trees. (Commutative)
     * @details
     * Depending on whether one of the nodes is an instance of sdf::ops::Round, i.e. a Node with a rounding operation
     * applied to it, the operation will remove the operation from the tree and apply the rounding in the intersection
     * with the same amount as used in the original rounding operation.
     */
    template<class A, class B>
    std::shared_ptr<sdf::ops::Union>
    operator+(const std::shared_ptr<A> &a, const std::shared_ptr<B> &b) requires std::is_base_of_v<Node, A> &&
                                                                                 std::is_base_of_v<Node, B> {
        if constexpr (std::is_same_v<sdf::ops::Round, A> && std::is_same_v<sdf::ops::Round, B>) {
            auto childA = a->getChild();
            float factorA = a->getRadius();
            auto childB = b->getChild();
            float factorB = b->getRadius();
            return Builder<sdf::ops::Union>(childA, childB, true, factorA + factorB).asNode();
        } else if constexpr (std::is_same_v<sdf::ops::Round, A>) {
            auto child = a->getChild();
            float factor = a->getRadius();
            return Builder<sdf::ops::Union>(child, b, true, factor).asNode();
        } else if constexpr (std::is_same_v<sdf::ops::Round, B>) {
            auto child = b->getChild();
            float factor = b->getRadius();
            return Builder<sdf::ops::Union>(a, child, true, factor).asNode();
        }
        return Builder<sdf::ops::Union>(a, b).asNode();
    }

    /**
     * Obtain the Difference between two CSG trees. (NOT commutative)
     * @details
     * Depending on whether one of the nodes is an instance of sdf::ops::Round, i.e. a Node with a rounding operation
     * applied to it, the operation will remove the operation from the tree and apply the rounding in the intersection
     * with the same amount as used in the original rounding operation.
     */
    template<class A, class B>
    std::shared_ptr<sdf::ops::Difference>
    operator-(const std::shared_ptr<A> &a, const std::shared_ptr<B> &b) requires std::is_base_of_v<Node, A> &&
                                                                                 std::is_base_of_v<Node, B> {
        if constexpr (std::is_same_v<sdf::ops::Round, A> && std::is_same_v<sdf::ops::Round, B>) {
            auto childA = a->getChild();
            float factorA = a->getRadius();
            auto childB = b->getChild();
            float factorB = b->getRadius();
            return Builder<sdf::ops::Difference>(childA, childB, true, factorA + factorB).asNode();
        } else if constexpr (std::is_same_v<sdf::ops::Round, A>) {
            auto child = a->getChild();
            float factor = a->getRadius();
            return Builder<sdf::ops::Difference>(child, b, true, factor).asNode();
        } else if constexpr (std::is_same_v<sdf::ops::Round, B>) {
            auto child = b->getChild();
            float factor = b->getRadius();
            return Builder<sdf::ops::Difference>(a, child, true, factor).asNode();
        }
        return Builder<sdf::ops::Difference>(a, b).asNode();
    }

    /**
     * Obtain the Intersection of two CSG trees. (Commutative)
     * @details
     * Depending on whether one of the nodes is an instance of sdf::ops::Round, i.e. a Node with a rounding operation
     * applied to it, the operation will remove the operation from the tree and apply the rounding in the intersection
     * with the same amount as used in the original rounding operation.
     */
    template<class A, class B>
    std::shared_ptr<sdf::ops::Intersection>
    operator|(const std::shared_ptr<A> &a, const std::shared_ptr<B> &b) requires std::is_base_of_v<Node, A> &&
                                                                                 std::is_base_of_v<Node, B> {
        if constexpr (std::is_same_v<sdf::ops::Round, A> && std::is_same_v<sdf::ops::Round, B>) {
            auto childA = a->getChild();
            float factorA = a->getRadius();
            auto childB = b->getChild();
            float factorB = b->getRadius();
            return Builder<sdf::ops::Intersection>(childA, childB, true, factorA + factorB).asNode();
        } else if constexpr (std::is_same_v<sdf::ops::Round, A>) {
            auto child = a->getChild();
            float factor = a->getRadius();
            return Builder<sdf::ops::Intersection>(child, b, true, factor).asNode();
        } else if constexpr (std::is_same_v<sdf::ops::Round, B>) {
            auto child = b->getChild();
            float factor = b->getRadius();
            return Builder<sdf::ops::Intersection>(a, child, true, factor).asNode();
        }
        return Builder<sdf::ops::Intersection>(a, b).asNode();
    }

    /**
     * Apply onioning to the product of a given CSG, producing a shell of the object. Produces multiple concentric
     * shells if applied consecutively.
     * @param a Root node of CSG tree to apply operation to
     * @param shell_thickness How thick the resulting object shell should be, in world distance
     */
    std::shared_ptr<sdf::ops::Onion> operator^(const std::shared_ptr<Node> &a, float shell_thickness) {
        return Builder<sdf::ops::Onion>(a, shell_thickness).asNode();
    }

    /**
     * Apply rounding to the product of a given CSG. Use in expressions where rounding should be subsumed by other
     * operators like Union, Intersection or Difference.
     * @param a Root node of CSG tree to apply operation to
     * @param amount How much the object should be rounded
     */
    std::shared_ptr<sdf::ops::Round> operator%(const std::shared_ptr<Node> &a, float amount) {
        return Builder<sdf::ops::Round>(a, amount).asNode();
    }

    /**
     * Apply rounding to the product of a given CSG. Use in expressions where rounding should NOT be subsumed by other
     * operators like Union, Intersection or Difference.
     * @param a Root node of CSG tree to apply operation to
     * @param amount How much the object should be rounded
     */
    std::shared_ptr<Node> operator%=(const std::shared_ptr<Node> &a, float amount) {
        return a % amount;
    }
}

#endif //PROJECT_UTILS_H

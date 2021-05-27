//
// Created by Yannis on 22/05/2021.
//

#ifndef PROJECT_UTILS_H
#define PROJECT_UTILS_H

namespace sdf {
    template<class T> requires std::is_base_of_v<Node, T>
    class Builder {
    private:
        std::shared_ptr<T> node;
        Material mat;
    public:
        template<typename ...Args> requires std::is_constructible_v<T, Args...>
        explicit Builder(Args &&... args) {
            node = std::make_shared<T>(std::forward<Args>(args)...);
        }

        Builder<T> withMaterial(const Material &material) {
            mat = material;
            return *this;
        }

        std::shared_ptr<T> build() {
            node->setMaterial(mat);
            return node;
        }
    };
}

#endif //PROJECT_UTILS_H

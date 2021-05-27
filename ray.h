//
// Created by Yannis on 06/04/2021.
//

#ifndef SECONDLAB_INTERSECTION_H
#define SECONDLAB_INTERSECTION_H

#include <glm/glm.hpp>
#include "camera.h"
#include "material.h"
#include <vector>

using glm::vec3;
using glm::mat3;
using std::vector;

class Ray {

public:
    Ray(const vec3& start, const vec3& dir) : start(start), dir(dir) {};

    [[nodiscard]] vec3 at(float t) const {
        return start + t * dir;
    }

    static Ray fromView(int x, int y, int w, int h, const std::shared_ptr<Camera>& camera) {
        auto d = vec3 (camera->rot() * vec4(x - w / 2.f, y - h / 2.f, camera->focalLength(), 0)) - camera->pos();
        return Ray(camera->pos(), glm::normalize(d));
    }
public:
    const vec3 start;
    const vec3 dir;
};

struct Hit {
    vec3 position;
    float t;
    vec3 normal;
    vec3 view;
    Material material;
};
#endif //SECONDLAB_INTERSECTION_H

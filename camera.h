//
// Created by Yannis on 07/04/2021.
//

#ifndef SECONDLAB_CAMERA_H
#define SECONDLAB_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

using glm::vec3;
using glm::vec4;
using glm::mat4;

class Camera {
public:
    Camera(const vec3& position, const vec3& up, float focalLength);
    void translate(float x, float y, float z, bool local=true);
    void rotate(const vec3& axis, float angle, bool local = true);

    [[nodiscard]] vec3 pos() const {
        return position;
    }
    [[nodiscard]] float focalLength() const {
        return f;
    }

    [[nodiscard]] mat4 rot() const {
        return R;
    }

    [[nodiscard]] mat4 transform() const {
        return R * T;
    }
private:
    mat4 T = mat4(1.f);
    mat4 R = mat4(1.f);
    vec3 forward;
    vec3 position;
    float f;
};

Camera::Camera(const vec3 &position, const vec3& up, float focalLength)
:
position(position),
f(focalLength)
{
    T = glm::lookAt(position, vec3(), up);
}

void Camera::translate(float x, float y, float z, bool local) {

    if (local) {
        T = glm::translate(T, vec3(R * vec4 {x, y, z, 0}));

    } else {
        T = glm::translate(T, vec3 {x, y, z});
    }
    position = vec3(T[3]);

}

void Camera::rotate(const vec3 &axis, float angle, bool local) {
    R = glm::rotate(R, angle, axis);
}

#endif //SECONDLAB_CAMERA_H

//
// Created by Yannis on 07/04/2021.
//

#ifndef SECONDLAB_LIGHT_H
#define SECONDLAB_LIGHT_H

#include <glm/glm.hpp>

using glm::vec3;

class Light {
public:
    Light(const vec3 &position, const vec3 &color, float intensity = 1.f) : position(position), color(color),
                                                                      intensity(intensity > maxIntensity ? maxIntensity : intensity) {}

public:
    vec3 position;
    vec3 color;
    float intensity;
    static constexpr float maxIntensity = 100;
};

#endif //SECONDLAB_LIGHT_H

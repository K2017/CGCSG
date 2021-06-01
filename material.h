//
// Created by Yannis on 14/05/2021.
//

#ifndef PROJECT_MATERIAL_H
#define PROJECT_MATERIAL_H

#include <glm/glm.hpp>

class Material {
public:

    /** Surface color
     * @details
     * Components: 0 to 1 inclusive
     */
    glm::vec3 albedo{0, 0, 0};

    /** Diffuse coefficient
     * @details
     * 0 to 1 inclusive
     */
    float kd = 1;

    /** Ambient coefficient
     * @details
     * 0 to 1 inclusive
     */
    float ka = 0.1f;

    /** Specular coefficient
     * @details
     * 0 to 1 inclusive
     */
    float ks = 0;

    /** Specular power
     * @details
     * 1 to 256 inclusive
     */
    float p = 4.0f;

    /** Index of refraction
     * @details
     * Air = 1
     * Glass = 1.5
     * Water = 1.33
     */
    float ior = 1;

    /** Coefficient of Transmittance
     * @details
     * 0 to 1 inclusive
     */
    float transmittance = 0.0f;

    /** How much light the inner material absorbs
     * @details
     * Percentage from 0 to 1
     */
    float absorption = 0.0f;

    // Linearly interpolate between materials
    static Material mix(const Material& a, const Material& b, float factor) {
        Material mixed;
        mixed.albedo = glm::mix(a.albedo, b.albedo, factor);
        mixed.kd = glm::mix(a.kd, b.kd, factor);
        mixed.ka = glm::mix(a.ka, b.ka, factor);
        mixed.ks = glm::mix(a.ks, b.ks, factor);
        mixed.p = glm::mix(a.p, b.p, factor);
        mixed.ior = glm::mix(a.ior, b.ior, factor);
        mixed.transmittance = glm::mix(a.transmittance, b.transmittance, factor);
        mixed.absorption = glm::mix(a.absorption, b.absorption, factor);
        return mixed;
    }

    static Material Default() {
        return {
            .albedo{0.8, 0.8, 0.8},
            .kd = 0.8
        };
    }
};

#endif //PROJECT_MATERIAL_H

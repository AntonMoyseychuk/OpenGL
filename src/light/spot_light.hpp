#pragma once
#include "light.hpp"

struct spot_light : public light {
    spot_light() = default;
    spot_light(const glm::vec3& color, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& position, const glm::vec3& direction, float cutoff);

    glm::vec3 position;
    glm::vec3 direction;

    float cutoff;
};
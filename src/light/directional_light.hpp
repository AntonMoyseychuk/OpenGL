#pragma once
#include "light.hpp"

struct directional_light : public light {
    directional_light() = default;
    directional_light(const glm::vec3& color, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& direction);

    glm::vec3 direction;
};
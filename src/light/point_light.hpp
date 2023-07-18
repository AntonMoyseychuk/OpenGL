#pragma once
#include "light.hpp"

struct point_light : public light {
    point_light() = default;
    point_light(const glm::vec3& color, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& position, float linear, float quadratic);

    glm::vec3 position;
    float linear;
    float quadratic;
};
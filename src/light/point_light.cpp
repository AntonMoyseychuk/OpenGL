#include "point_light.hpp"

point_light::point_light(const glm::vec3& color, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& position, float linear, float quadratic)
    : light(color, ambient, diffuse, specular), position(position), linear(linear), quadratic(quadratic)
{
}

#include "directional_light.hpp"

directional_light::directional_light(const glm::vec3& color, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& direction)
    : light(color, ambient, diffuse, specular), direction(direction)
{
}
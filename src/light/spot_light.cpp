#include "spot_light.hpp"

spot_light::spot_light(const glm::vec3& color, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& position, const glm::vec3& direction, float cutoff)
    : light(color, ambient, diffuse, specular), position(position), direction(direction), cutoff(cutoff)
{
}
#include "uv_sphere.hpp"
#include <glm/gtc/constants.hpp>

uv_sphere::uv_sphere(uint32_t stacks, uint32_t slices) {
    generate(stacks, slices);
}

void uv_sphere::generate(uint32_t stacks, uint32_t slices) noexcept {
    this->stacks = stacks;
    this->slices = slices;

    std::vector<mesh::vertex> vertices;
    vertices.reserve((stacks + 1) * (slices + 1));

    const float phi_step = glm::two_pi<float>() / slices;
    const float theta_step = glm::pi<float>() / stacks;

    for(uint32_t stack = 0; stack <= stacks; ++stack) {
        const float theta = stack * theta_step;
        const float xz = glm::sin(theta); 
        const float y = glm::cos(theta); 

        for(uint32_t slice = 0; slice <= slices; ++slice) {
            const float phi = slice * phi_step; 

            const float x = xz * glm::cos(phi); 
            const float z = xz * glm::sin(phi);

            const glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
            const glm::vec2 texcoord(static_cast<float>(slice) / slices, static_cast<float>(stack) / stacks);

            vertices.emplace_back(mesh::vertex{glm::vec3(x, y, z), normal, texcoord});
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve((2 * slices * 3) + ((stacks - 2) * slices * 6));

    for(uint32_t stack = 0; stack < stacks; ++stack) {
        uint32_t k1 = stack * (slices + 1);
        uint32_t k2 = k1 + slices + 1;

        for(uint32_t slice = 0; slice < slices; ++slice, ++k1, ++k2) {
            if(stack != 0) {
                indices.emplace_back(k1 + 1);
                indices.emplace_back(k2);
                indices.emplace_back(k1);
            }

            if(stack != stacks - 1) {
                indices.emplace_back(k2 + 1);
                indices.emplace_back(k2);
                indices.emplace_back(k1 + 1);
            }
        }
    }

    ASSERT(indices.size() >= 3, "uv sphere generation error", "index count < 3");
    ASSERT(indices.size() % 3 == 0, "uv sphere generation error", "index count % 3 != 0");

    mesh.create(vertices, indices);
}

void uv_sphere::add_texture(texture_2d&& tex) noexcept {
    mesh.add_texture(std::forward<texture_2d>(tex));
}

mesh::vertex uv_sphere::_create_vertex(float theta, float phi, float tex_s, float tex_t) const noexcept {
    mesh::vertex v;
    v.position.x = glm::sin(theta) * glm::sin(phi);
    v.position.y = glm::cos(theta);
    v.position.z = glm::sin(theta) * glm::cos(phi);

    v.texcoord.x = tex_s;
    v.texcoord.y = tex_t;

    return v;
}

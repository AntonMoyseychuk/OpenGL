#include "mesh.hpp"
#include "debug.hpp"

#include <glad/glad.h>
#include <utility>

// mesh::vertex::vertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& texcoord, const glm::vec3& tangent)
//     : position(position), normal(normal), texcoord(texcoord), tangent(tangent)
// {
// }

mesh::mesh(const std::vector<vertex> &vertices, const std::vector<uint32_t> &indices) {
    create(vertices, indices);
}

void mesh::create(const std::vector<vertex> &vertices, const std::vector<uint32_t> &indices) noexcept {
    vao.create();
    vao.bind();

    vbo.create(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), sizeof(vertex), GL_STATIC_DRAW, &vertices[0]);

    vao.set_attribute(vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
    vao.set_attribute(vbo, 1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal));
    vao.set_attribute(vbo, 2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texcoord));
    vao.set_attribute(vbo, 3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tangent));

    if (!indices.empty()) {
        ibo.create(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), sizeof(indices[0]), GL_STATIC_DRAW, &indices[0]);
        ibo.bind();
    }

    vao.unbind();
}

void mesh::bind(const shader &shader) const noexcept {
    size_t simple_texture = 0;
    size_t diffuse_number = 0, specular_number = 0, emission_number = 0, normal_number = 0;
    
    shader.bind();
    int32_t i = 0;
    for (const auto& texture : textures) {
        texture.bind(i);

        switch (texture.get_variety()) {
        case texture_2d::variety::NONE:
            shader.uniform(("u_material.texture" + std::to_string(simple_texture++)).c_str(), i);
            break;
        case texture_2d::variety::DIFFUSE:
            shader.uniform(("u_material.diffuse" + std::to_string(diffuse_number++)).c_str(), i);
            break;
        case texture_2d::variety::SPECULAR:
            shader.uniform(("u_material.specular" + std::to_string(specular_number++)).c_str(), i);
            break;
        case texture_2d::variety::EMISSION:
            shader.uniform(("u_material.emission" + std::to_string(emission_number++)).c_str(), i);
            break;
        case texture_2d::variety::NORMAL:
            shader.uniform(("u_material.normal" + std::to_string(normal_number++)).c_str(), i);
            break;
        default:
            ASSERT(false, "mesh drawing error", "invalid texture type");
            break;
        }

        ++i;
    }

    vao.bind();
}

void mesh::add_texture(texture_2d &&tex) noexcept {
    textures.push_back(std::forward<texture_2d>(tex));
}

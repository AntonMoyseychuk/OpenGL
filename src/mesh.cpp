#include "mesh.hpp"
#include "debug.hpp"

#include <glad/glad.h>
#include <utility>

mesh::mesh(const std::vector<mesh::vertex> &vertices, const std::vector<uint32_t> &indices) {
    create(vertices, indices);
}

void mesh::create(const std::vector<vertex> &vertices, const std::vector<uint32_t> &indices) noexcept {
    m_vao.create();
    m_vao.bind();

    m_vbo.create(GL_ARRAY_BUFFER, vertices.size(), sizeof(vertex), GL_STATIC_DRAW, &vertices[0]);

    m_vao.set_attribute(m_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
    m_vao.set_attribute(m_vbo, 1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal));
    m_vao.set_attribute(m_vbo, 2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texcoord));

    if (!indices.empty()) {
        m_ibo.create(GL_ELEMENT_ARRAY_BUFFER, indices.size(), sizeof(indices[0]), GL_STATIC_DRAW, &indices[0]);
        m_ibo.bind();
    }

    m_vao.unbind();

    // set_textures(texture_configs);
}

void mesh::bind(const shader &shader) const noexcept {
    size_t simple_texture = 0;
    size_t diffuse_number = 0, specular_number = 0, emission_number = 0, normal_number = 0;
    
    shader.bind();
    int32_t i = 0;
    for (const auto& texture : m_textures) {
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
        case texture_2d::variety::HEIGHT:
            shader.uniform(("u_material.height" + std::to_string(normal_number++)).c_str(), i);
            break;
        default:
            ASSERT(false, "mesh drawing error", "invalid texture type");
            break;
        }

        ++i;
    }

    m_vao.bind();
}

void mesh::add_texture(texture_2d &&tex) noexcept {
    m_textures.push_front(std::forward<texture_2d>(tex));
}

// void mesh::set_textures(const std::unordered_map<std::string, texture::data> &texture_configs) noexcept {
//     if (!m_textures.empty()) {
//         m_textures.resize(0);
//     }
//     m_textures.reserve(texture_configs.size());
//     for (const auto& [filepath, config] : texture_configs) {
//         m_textures.emplace_back(filepath, config);
//     }
// }

const buffer& mesh::get_vertex_buffer() const noexcept {
    return m_vbo;
}

const buffer& mesh::get_index_buffer() const noexcept {
    return m_ibo;
}

const vertex_array& mesh::get_vertex_array() const noexcept {
    return m_vao;
}

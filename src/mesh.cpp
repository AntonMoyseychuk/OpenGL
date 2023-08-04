#include "mesh.hpp"
#include "debug.hpp"

#include <glad/glad.h>

mesh::mesh(const std::vector<mesh::vertex> &vertices, const std::vector<uint32_t> &indices, 
    const std::unordered_map<std::string, texture::config>& textures) 
{
    create(vertices, indices, textures);
}

void mesh::create(const std::vector<vertex> &vertices, const std::vector<uint32_t> &indices, 
    const std::unordered_map<std::string, texture::config>& textures) noexcept 
{
    m_vao.create();
    m_vao.bind();

    m_vbo.create(GL_ARRAY_BUFFER, vertices.size(), sizeof(vertex), GL_STATIC_DRAW, &vertices[0]);
    m_vbo.bind();

    m_vao.enable_attribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
    m_vao.enable_attribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal));
    m_vao.enable_attribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texcoord));

    if (!indices.empty()) {
        m_ibo.create(GL_ELEMENT_ARRAY_BUFFER, indices.size(), sizeof(indices[0]), GL_STATIC_DRAW, &indices[0]);
        m_ibo.bind();
    }

    m_vao.unbind();

    if (!m_textures.empty()) {
        m_textures.resize(0);
    }
    m_textures.reserve(textures.size());
    for (const auto& [filepath, config] : textures) {
        m_textures.emplace_back(filepath, config);
    }
}

void mesh::set_textures(const std::vector<texture> &textures) noexcept {
    m_textures = textures;
}

void mesh::draw(const shader& shader) const noexcept {
    size_t simple_texture = 0;
    size_t diffuse_number = 0, specular_number = 0, emission_number = 0, normal_number = 0;
    
    shader.bind();
    for (int32_t i = 0; i < m_textures.size(); ++i) {
        m_textures[i].bind(i);

        switch (m_textures[i].get_type()) {
        case texture::type::NONE:
            shader.uniform(("u_material.texture" + std::to_string(simple_texture++)).c_str(), i);
            break;
        case texture::type::DIFFUSE:
            shader.uniform(("u_material.diffuse" + std::to_string(diffuse_number++)).c_str(), i);
            break;
        case texture::type::SPECULAR:
            shader.uniform(("u_material.specular" + std::to_string(specular_number++)).c_str(), i);
            break;
        case texture::type::EMISSION:
            shader.uniform(("u_material.emission" + std::to_string(emission_number++)).c_str(), i);
            break;
        case texture::type::NORMAL:
            shader.uniform(("u_material.normal" + std::to_string(normal_number++)).c_str(), i);
            break;
        default:
            ASSERT(false, "mesh drawing error", "invalid texture type");
            break;
        }
    }

    m_vao.bind();
    if (m_ibo.get_element_count() == 0) {
        glDrawArrays(GL_TRIANGLES, 0, m_vbo.get_element_count());
    } else {
        glDrawElements(GL_TRIANGLES, m_ibo.get_element_count(), GL_UNSIGNED_INT, 0);
    }
    m_vao.unbind();
}

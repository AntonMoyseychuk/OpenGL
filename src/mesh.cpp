#include "mesh.hpp"
#include "debug.hpp"

#include <glad/glad.h>

mesh::mesh(const std::vector<mesh::vertex> &vertices, const std::vector<uint32_t> &indices, const std::vector<texture> &textures) {
    create(vertices, indices, textures);
}

void mesh::create(const std::vector<vertex> &vertices, const std::vector<uint32_t> &indices, const std::vector<texture> &textures) noexcept {
    m_vertex_count = vertices.size();
    m_index_count = indices.size();
    m_textures = textures;
    
    OGL_CALL(glGenVertexArrays(1, &m_vao));
    OGL_CALL(glGenBuffers(1, &m_vbo));
    OGL_CALL(glGenBuffers(1, &m_ebo));
  
    OGL_CALL(glBindVertexArray(m_vao));
    OGL_CALL(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    OGL_CALL(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), &vertices[0], GL_STATIC_DRAW));  
    OGL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0));
    OGL_CALL(glEnableVertexAttribArray(0));	
    OGL_CALL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal)));
    OGL_CALL(glEnableVertexAttribArray(1));	
    OGL_CALL(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texcoord)));
    OGL_CALL(glEnableVertexAttribArray(2));	

    if (!indices.empty()) {
        OGL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo));
        OGL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices[0], GL_STATIC_DRAW));
    }

    OGL_CALL(glBindVertexArray(0));
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

    glBindVertexArray(m_vao);
    if (m_index_count == 0) {
        glDrawArrays(GL_TRIANGLES, 0, m_vertex_count);
    } else {
        glDrawElements(GL_TRIANGLES, m_index_count, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

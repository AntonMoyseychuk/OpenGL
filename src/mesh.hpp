#pragma once
#include "shader.hpp"
#include "texture.hpp"

#include "buffer.hpp"
#include "vertex_array.hpp"

#include <glm/glm.hpp>

#include <vector>


class mesh {
public:
    struct vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };

public:
    mesh() = default;
    mesh(const std::vector<mesh::vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<texture>& textures);
    
    void create(const std::vector<mesh::vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<texture>& textures) noexcept;
    void set_textures(const std::vector<texture>& textures) noexcept;

    void draw(const shader& shader) const noexcept;

private:
    std::vector<texture> m_textures;

    buffer m_vbo;
    buffer m_ibo;
    vertex_array m_vao;
};
#pragma once
#include "shader.hpp"
#include "texture.hpp"

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
    void create(const std::vector<mesh::vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<texture>& textures) const noexcept;

    void draw(const shader& shader) const noexcept;

private:
    std::vector<texture> m_textures;
    size_t m_vertex_count = 0;
    size_t m_index_count = 0;

    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
};
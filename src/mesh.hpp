#pragma once
#include "shader.hpp"
#include "texture.hpp"

#include "buffer.hpp"
#include "vertex_array.hpp"

#include <glm/glm.hpp>

#include <vector>

#include "nocopyable.hpp"

struct mesh : public nocopyable {
    struct vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoord;
        glm::vec3 tangent;
    };

    mesh() = default;
    mesh(const std::vector<vertex>& vertices, const std::vector<uint32_t>& indices);
    
    void create(const std::vector<vertex>& vertices, const std::vector<uint32_t>& indices) noexcept;
    
    void bind(const shader& shader) const noexcept;

    void add_texture(texture_2d&& texture) noexcept;

    std::vector<texture_2d> textures;
    buffer vbo;
    buffer ibo;
    vertex_array vao;
};
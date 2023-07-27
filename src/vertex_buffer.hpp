#pragma once
#include <glad/glad.h>
#include <cstdint>

class vertex_buffer {
public:
    vertex_buffer() = default;
    vertex_buffer(const void* data, size_t vertex_count, size_t vertex_size, decltype(GL_STATIC_DRAW) usage = GL_STATIC_DRAW);

    ~vertex_buffer();

    void create(const void* data, size_t vertex_count, size_t vertex_size, decltype(GL_STATIC_DRAW) usage = GL_STATIC_DRAW) noexcept;
    void destroy() noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    void add_attribute(uint32_t index, uint32_t size, decltype(GL_FLOAT) type, bool normalized, size_t stride, const void* pointer) const noexcept;

    size_t get_vertex_count() const noexcept;

    vertex_buffer(vertex_buffer&& vbo);
    vertex_buffer& operator=(vertex_buffer&& vbo) noexcept;

    vertex_buffer(const vertex_buffer& vbo) = delete;
    vertex_buffer& operator=(const vertex_buffer& vbo) = delete;

private:
    size_t m_vertex_count = 0;
    size_t m_vertex_size = 0;
    uint32_t m_id = 0;
    decltype(GL_STATIC_DRAW) m_usage = 0;
};
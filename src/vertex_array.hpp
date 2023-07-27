#pragma once
#include <glad/glad.h>
#include <cstdint>

class vertex_array {
public:
    vertex_array() = default;
    ~vertex_array();

    void create() noexcept;
    void destroy() noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    vertex_array(vertex_array&& vao);
    vertex_array& operator=(vertex_array&& vao) noexcept;

    vertex_array(const vertex_array& vao) = delete;
    vertex_array& operator=(const vertex_array& vao) = delete;

private:
    uint32_t m_id = 0;
};
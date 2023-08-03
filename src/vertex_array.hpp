#pragma once
#include <cstdint>

class vertex_array {
public:
    vertex_array() = default;
    ~vertex_array();

    void create() noexcept;
    void destroy() noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    void enable_attribute(uint32_t index, uint32_t size, uint32_t type, bool normalized, uint32_t stride, const void* pointer) const noexcept;
    void disable_attribute(uint32_t index) const noexcept;

    vertex_array(vertex_array&& vao);
    vertex_array& operator=(vertex_array&& vao) noexcept;

    vertex_array(const vertex_array& vao) = delete;
    vertex_array& operator=(const vertex_array& vao) = delete;

private:
    uint32_t m_id = 0;
};
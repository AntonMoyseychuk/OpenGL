#pragma once
#include "buffer.hpp"

#include "nocopyable.hpp"

class vertex_array : public nocopyable {
public:
    vertex_array() = default;
    ~vertex_array();

    void create() noexcept;
    void destroy() noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    void set_attribute(const buffer& buffer, uint32_t index, uint32_t size, uint32_t type, bool normalized, uint32_t stride, const void* pointer) const noexcept;
    void remove_attribute(uint32_t index) const noexcept;
    
    void set_attribute_divisor(uint32_t index, uint32_t divisor) const noexcept;
    uint32_t get_id() const noexcept;

    vertex_array(vertex_array&& vao);
    vertex_array& operator=(vertex_array&& vao) noexcept;

private:
    uint32_t m_id = 0;
};
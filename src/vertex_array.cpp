#include "vertex_array.hpp"

#include <glad/glad.h>

#include "debug.hpp"

vertex_array::~vertex_array() {
    destroy();
}

void vertex_array::create() noexcept {
    if (m_id != 0) {
        LOG_WARN("vertex array", "vertex array recreation (prev id = " + std::to_string(m_id) + ")");
        destroy();
    }

    OGL_CALL(glGenVertexArrays(1, &m_id));
}

void vertex_array::destroy() noexcept {
    OGL_CALL(glDeleteVertexArrays(1, &m_id));
    
    m_id = 0;
}

void vertex_array::bind() const noexcept {
    OGL_CALL(glBindVertexArray(m_id));
}

void vertex_array::unbind() const noexcept {
    OGL_CALL(glBindVertexArray(0));
}

void vertex_array::set_attribute(const buffer& buffer, uint32_t index, uint32_t size, uint32_t type, bool normalized, uint32_t stride, const void *pointer) const noexcept {
    bind();
    buffer.bind();
    OGL_CALL(glVertexAttribPointer(index, size, type, normalized, stride, pointer));
    OGL_CALL(glEnableVertexAttribArray(index));
}

void vertex_array::remove_attribute(uint32_t index) const noexcept {
    bind();
    OGL_CALL(glDisableVertexAttribArray(index));
}

void vertex_array::set_attribute_divisor(uint32_t index, uint32_t divisor) const noexcept {
    bind();
    OGL_CALL(glVertexAttribDivisor(index, divisor));
}

uint32_t vertex_array::get_id() const noexcept {
    return m_id;
}

vertex_array::vertex_array(vertex_array &&vao)
    : m_id(vao.m_id)
{
    vao.m_id = 0;
}

vertex_array &vertex_array::operator=(vertex_array &&vao) noexcept {
    m_id = vao.m_id;
    vao.m_id = 0;

    return *this;
}

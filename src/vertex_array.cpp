#include "vertex_array.hpp"

#include "debug.hpp"

vertex_array::~vertex_array() {
    this->destroy();
}

void vertex_array::create() noexcept {
    if (m_id != 0) {
        LOG_WARN("vertex array", "vertex array recreation (id = " + std::to_string(m_id) + ")");
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

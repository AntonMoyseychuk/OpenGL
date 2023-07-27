#include "vertex_buffer.hpp"

#include "debug.hpp"

vertex_buffer::vertex_buffer(const void* data, size_t size, decltype(GL_STATIC_DRAW) usage) {
    this->create(data, size, usage);
}

vertex_buffer::~vertex_buffer() {
    _destroy();
}

void vertex_buffer::create(const void* data, size_t size, decltype(GL_STATIC_DRAW) usage) noexcept {
    if (m_id != 0) {
        LOG_WARN("vertex buffer", "vertex buffer recreation (id = " + std::to_string(m_id) + ")");
        _destroy();
    }

    m_size = size;
    m_usage = usage;

    glGenBuffers(1, &m_id);
    this->bind();
    glBufferData(GL_ARRAY_BUFFER, m_size, data, usage);

    this->unbind();
}

void vertex_buffer::bind() const noexcept {
    OGL_CALL(glBindBuffer(GL_ARRAY_BUFFER, m_id));
}

void vertex_buffer::unbind() const noexcept {
    OGL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void vertex_buffer::add_attribute(uint32_t index, uint32_t size, decltype(GL_FLOAT) type, bool normalized, size_t stride, const void *pointer) const noexcept {
    this->bind();
    OGL_CALL(glVertexAttribPointer(index, size, type, normalized, stride, pointer));
    OGL_CALL(glEnableVertexAttribArray(index));

    this->unbind();
}

vertex_buffer::vertex_buffer(vertex_buffer &&vbo)
    : m_id(vbo.m_id)
{
    memset(&vbo, 0, sizeof(vbo));
}

vertex_buffer &vertex_buffer::operator=(vertex_buffer &&vbo) noexcept {
    m_id = vbo.m_id;
    memset(&vbo, 0, sizeof(vbo));

    return *this;
}

void vertex_buffer::_destroy() const noexcept {
    OGL_CALL(glDeleteBuffers(1, &m_id));
    
    const_cast<vertex_buffer*>(this)->m_id = 0;
}
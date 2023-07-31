#include "vertex_buffer.hpp"

#include "debug.hpp"

vertex_buffer::vertex_buffer(const void* data, size_t vertex_count, size_t vertex_size, decltype(GL_STATIC_DRAW) usage) {
    this->create(data, vertex_count, vertex_size, usage);
}

vertex_buffer::~vertex_buffer() {
    destroy();
}

void vertex_buffer::create(const void* data, size_t vertex_count, size_t vertex_size, decltype(GL_STATIC_DRAW) usage) noexcept {
    if (m_id != 0) {
        LOG_WARN("vertex buffer", "vertex buffer recreation (id = " + std::to_string(m_id) + ")");
        destroy();
    }

    m_vertex_count = vertex_count;
    m_vertex_size = vertex_size;

    OGL_CALL(glGenBuffers(1, &m_id));
    this->bind();
    OGL_CALL(glBufferData(GL_ARRAY_BUFFER, m_vertex_count * m_vertex_size, data, usage));

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

size_t vertex_buffer::get_vertex_count() const noexcept {
    return m_vertex_count;
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

void vertex_buffer::destroy() noexcept {
    OGL_CALL(glDeleteBuffers(1, &m_id));
    
    m_id = 0;
}
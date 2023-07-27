#include "index_buffer.hpp"

#include "debug.hpp"

index_buffer::index_buffer(const void* data, size_t count, size_t index_size, decltype(GL_STATIC_DRAW) usage) {
    this->create(data, count, index_size, usage);
}

index_buffer::~index_buffer() {
    this->destroy();
}

void index_buffer::create(const void* data, size_t count, size_t index_size, decltype(GL_STATIC_DRAW) usage) noexcept {
    if (m_id != 0) {
        LOG_WARN("index buffer", "index buffer recreation (id = " + std::to_string(m_id) + ")");
        destroy();
    }

    m_count = count;
    m_usage = usage;

    OGL_CALL(glGenBuffers(1, &m_id));
    this->bind();
    OGL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * index_size, data, usage));

    this->unbind();
}

void index_buffer::destroy() noexcept {
    OGL_CALL(glDeleteBuffers(1, &m_id));
    
    m_id = 0;
}

void index_buffer::bind() const noexcept {
    OGL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id));
}

void index_buffer::unbind() const noexcept {
    OGL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id));
}

size_t index_buffer::get_index_count() const noexcept {
    return m_count;
}

index_buffer::index_buffer(index_buffer &&ibo)
    : m_id(ibo.m_id), m_count(ibo.m_count)
{
    memset(&ibo, 0, sizeof(ibo));
}

index_buffer &index_buffer::operator=(index_buffer &&ibo) noexcept {
    m_id = ibo.m_id;
    m_count = ibo.m_count;
    memset(&ibo, 0, sizeof(ibo));

    return *this;
}
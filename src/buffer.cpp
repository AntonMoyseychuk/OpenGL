#include <glad/glad.h>

#include "buffer.hpp"
#include "debug.hpp"

buffer::buffer(uint32_t target, size_t element_count, size_t element_size, uint32_t usage, const void *data) {
    create(target, element_count, element_size, usage, data);
}

buffer::~buffer() {
    destroy();
}

void buffer::create(uint32_t target, size_t element_count, size_t element_size, uint32_t usage, const void *data) noexcept {
    if (m_id != 0) {
        LOG_WARN(_target_to_string(target) + " buffer", _target_to_string(target) + "buffer recreation (prev id = " + std::to_string(m_id) + ")");
        destroy();
    }

    m_target = target;
    m_element_count = element_count;
    m_element_size = element_size;
    m_usage = usage;

    OGL_CALL(glGenBuffers(1, &m_id));
    bind();
    OGL_CALL(glBufferData(m_target, m_element_count * m_element_size, data, usage));
}

void buffer::destroy() noexcept {
    OGL_CALL(glDeleteBuffers(1, &m_id));
    m_id = 0;
}

void buffer::subdata(uint32_t offset, uint32_t size, const void* data) const noexcept {
    bind();
    OGL_CALL(glBufferSubData(m_target, offset, size, data));
}

void buffer::bind() const noexcept {
    OGL_CALL(glBindBuffer(m_target, m_id));
}

void buffer::unbind() const noexcept {
    OGL_CALL(glBindBuffer(m_target, 0));
}

size_t buffer::get_element_count() const noexcept {
    return m_element_count;
}

size_t buffer::get_element_size() const noexcept {
    return m_element_size;
}

uint32_t buffer::get_target() const noexcept {
    return m_target;
}

uint32_t buffer::get_usage() const noexcept {
    return m_usage;
}

uint32_t buffer::get_id() const noexcept {
    return m_id;
}

buffer::buffer(buffer&& buffer) noexcept
    : m_element_count(buffer.m_element_count), m_element_size(buffer.m_element_size), m_target(buffer.m_target), m_usage(buffer.m_usage), m_id(buffer.m_id)
{
    memset(&buffer, 0, sizeof(buffer));
}

buffer& buffer::operator=(buffer&& buffer) noexcept {
    m_element_count = buffer.m_element_count;
    m_element_size = buffer.m_element_size; 
    m_target = buffer.m_target;
    m_usage = buffer.m_usage;
    m_id = buffer.m_id;

    memset(&buffer, 0, sizeof(buffer));

    return *this;
}

const std::string& buffer::_target_to_string(uint32_t target) noexcept {
    static const std::unordered_map<uint32_t, std::string> strings = {
        std::make_pair(GL_ARRAY_BUFFER, "vertex"),
        std::make_pair(GL_ELEMENT_ARRAY_BUFFER, "index"),
        std::make_pair(GL_UNIFORM_BUFFER, "uniform")
    };

    static const std::string unrecognized = "unrecognized";

    return strings.find(target) != strings.cend() ? strings.at(target) : unrecognized;
}

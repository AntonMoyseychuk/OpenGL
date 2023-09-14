#include "renderbuffer.hpp"

#include <glad/glad.h>

#include "debug.hpp"


renderbuffer::renderbuffer(uint32_t width, uint32_t height, uint32_t internal_format) {
    create(width, height, internal_format);
}

renderbuffer::~renderbuffer() {
    destroy();
}

void renderbuffer::create(uint32_t width, uint32_t height, uint32_t internal_format) noexcept {
    if (m_id != 0) {
        LOG_WARN("renderbuffer warning", "renderbuffer recreation (prev id = " + std::to_string(m_id) + ")");
        destroy();
    }

    m_width = width;
    m_height = height;
    m_internal_format = internal_format;

    OGL_CALL(glGenRenderbuffers(1, &m_id));
    bind();
    OGL_CALL(glRenderbufferStorage(GL_RENDERBUFFER, m_internal_format, m_width, m_height));
}

void renderbuffer::destroy() noexcept {
    OGL_CALL(glDeleteRenderbuffers(1, &m_id));

    m_id = 0;
}

void renderbuffer::bind() const noexcept {
    OGL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, m_id));
}

void renderbuffer::unbind() const noexcept {
    OGL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

uint32_t renderbuffer::get_id() const noexcept {
    return m_id;
}

uint32_t renderbuffer::get_width() const noexcept {
    return m_width;
}

uint32_t renderbuffer::get_height() const noexcept {
    return m_height;
}

uint32_t renderbuffer::get_internal_format() const noexcept {
    return m_internal_format;
}

renderbuffer::renderbuffer(renderbuffer &&renderbuffer) 
    : m_id(renderbuffer.m_id), m_width(renderbuffer.m_width), m_height(renderbuffer.m_height), m_internal_format(renderbuffer.m_internal_format)
{
    if (this != &renderbuffer) {
        memset(&renderbuffer, 0, sizeof(renderbuffer));
    }
}

renderbuffer &renderbuffer::operator=(renderbuffer &&renderbuffer) noexcept {
    if (this != &renderbuffer) {
        m_id = renderbuffer.m_id;
        m_width = renderbuffer.m_width;
        m_height = renderbuffer.m_height;
        m_internal_format = renderbuffer.m_internal_format;

        memset(&renderbuffer, 0, sizeof(renderbuffer));
    }

    return *this;
}

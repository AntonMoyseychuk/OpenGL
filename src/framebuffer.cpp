#include "framebuffer.hpp"

#include <glad/glad.h>

#include "debug.hpp"

framebuffer::~framebuffer() {
    destroy();
}

void framebuffer::create() noexcept {
    if (m_id != 0) {
        LOG_WARN("framebuffer warning", "framebuffer recreation (prev id = " + std::to_string(m_id) + ")");
        destroy();
    }

    OGL_CALL(glGenFramebuffers(1, &m_id));
}

void framebuffer::destroy() noexcept {
    OGL_CALL(glDeleteFramebuffers(1, &m_id));

    m_id = 0;
}

void framebuffer::bind() const noexcept {
    OGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, m_id));
}

void framebuffer::unbind() const noexcept {
    OGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void framebuffer::bind_default() noexcept {
    OGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

uint32_t framebuffer::get_id() const noexcept {
    return m_id;
}

bool framebuffer::attach(uint32_t attachment, uint32_t level, const texture_2d &texture) const noexcept {
    bind();

    OGL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texture.get_target(), texture.get_id(), level));
    OGL_CALL(m_is_complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE));

    return m_is_complete;
}

bool framebuffer::attach(uint32_t attachment, uint32_t level, const cubemap &cubemap) const noexcept {
    bind();

    OGL_CALL(glFramebufferTexture(GL_FRAMEBUFFER, attachment, cubemap.get_id(), level));
    OGL_CALL(m_is_complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE));

    return m_is_complete;
}

bool framebuffer::attach(uint32_t attachment, const renderbuffer &renderbuffer) const noexcept {
    bind();

    OGL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer.get_id()));

    return m_is_complete;
}

void framebuffer::set_draw_buffer(uint32_t buffer) const noexcept {
    OGL_CALL(glNamedFramebufferDrawBuffer(m_id, buffer));
}

void framebuffer::set_draw_buffer(size_t count, const uint32_t *buffers) const noexcept {
    OGL_CALL(glNamedFramebufferDrawBuffers(m_id, count, buffers));
}

void framebuffer::set_read_buffer(uint32_t src) const noexcept {
    OGL_CALL(glNamedFramebufferReadBuffer(m_id, src));
}

bool framebuffer::is_complete() const noexcept {
    return m_is_complete;
}

framebuffer::framebuffer(framebuffer &&framebuffer)
    : m_id(framebuffer.m_id), m_is_complete(framebuffer.m_is_complete)
{
    if (this != &framebuffer) {
        framebuffer.m_id = 0;
        framebuffer.m_is_complete = false;
    }
}

framebuffer &framebuffer::operator=(framebuffer &&framebuffer) noexcept {
    if (this != &framebuffer) {
        m_id = framebuffer.m_id;
        m_is_complete = framebuffer.m_is_complete;
        
        framebuffer.m_id = 0;
        framebuffer.m_is_complete = false;
    }
    
    return *this;
}

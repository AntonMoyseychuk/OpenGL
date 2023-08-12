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

bool framebuffer::attach(uint32_t attachment, const texture &texture) noexcept {
    bind();

    ASSERT(texture.get_config_data().target == GL_TEXTURE_2D, "framebuffer error", "can't attach not GL_TEXTURE_2D textures yet");
    OGL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texture.get_config_data().target, texture.get_id(), texture.get_config_data().level));
    OGL_CALL(m_is_complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE));

    return is_complete();
}

bool framebuffer::attach(uint32_t attachment, const renderbuffer &renderbuffer) noexcept {
    bind();

    OGL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer.get_id()));

    return is_complete();
}

void framebuffer::set_draw_buffer(uint32_t buffer) noexcept {
    OGL_CALL(glDrawBuffer(buffer));
}

void framebuffer::set_read_buffer(uint32_t src) noexcept {
    OGL_CALL(glReadBuffer(src));
}

bool framebuffer::is_complete() const noexcept {
    return m_is_complete;
}

framebuffer::framebuffer(framebuffer &&framebuffer)
    : m_id(framebuffer.m_id), m_is_complete(framebuffer.m_is_complete)
{
    framebuffer.m_id = 0;
    framebuffer.m_is_complete = false;
}

framebuffer &framebuffer::operator=(framebuffer &&framebuffer) noexcept {
    m_id = framebuffer.m_id;
    m_is_complete = framebuffer.m_is_complete;
    
    framebuffer.m_id = 0;
    framebuffer.m_is_complete = false;
    
    return *this;
}

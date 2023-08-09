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

void framebuffer::attach(uint32_t attachment, const texture &texture) noexcept {
    bind();

    ASSERT(texture.get_config_data().target == GL_TEXTURE_2D, "framebuffer error", "can't attach not GL_TEXTURE_2D textures yet");
    OGL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texture.get_config_data().target, texture.get_id(), texture.get_config_data().level));
    OGL_CALL(m_is_complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE));
}

void framebuffer::attach(uint32_t attachment, const renderbuffer &renderbuffer) noexcept {
    bind();

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer.get_id());
}

bool framebuffer::is_complete() const noexcept {
    return m_is_complete;
}

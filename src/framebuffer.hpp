#pragma once
#include "texture.hpp"
#include "renderbuffer.hpp"

class framebuffer {
public:
    framebuffer() = default;
    ~framebuffer();

    void create() noexcept;
    void destroy() noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    void attach(uint32_t attachment, const texture& texture) noexcept;
    void attach(uint32_t attachment, const renderbuffer& renderbuffer) noexcept;

    bool is_complete() const noexcept;

private:
    uint32_t m_id = 0;
    bool m_is_complete = false;
};
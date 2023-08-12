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

    bool attach(uint32_t attachment, const texture& texture) noexcept;
    bool attach(uint32_t attachment, const renderbuffer& renderbuffer) noexcept;

    void set_draw_buffer(uint32_t buffer) noexcept;
    void set_read_buffer(uint32_t src) noexcept;

    bool is_complete() const noexcept;


    framebuffer(framebuffer&& framebuffer);
    framebuffer& operator=(framebuffer&& framebuffer) noexcept;

    framebuffer(const framebuffer& framebuffer) = delete;
    framebuffer& operator=(const framebuffer& framebuffer) = delete;

private:
    uint32_t m_id = 0;
    bool m_is_complete = false;
};
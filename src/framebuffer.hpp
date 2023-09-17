#pragma once
#include "texture.hpp"
#include "cubemap.hpp"
#include "renderbuffer.hpp"

class framebuffer {
public:
    framebuffer() = default;
    ~framebuffer();

    void create() noexcept;
    void destroy() noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    static void bind_default() noexcept;

    uint32_t get_id() const noexcept;

    bool attach(uint32_t attachment, uint32_t level, const texture_2d& texture) const noexcept;
    bool attach(uint32_t attachment, uint32_t level, const cubemap& cubemap) const noexcept;
    bool attach(uint32_t attachment, const renderbuffer& renderbuffer) const noexcept;

    void set_draw_buffer(uint32_t buffer) const noexcept;
    void set_draw_buffer(size_t count, const uint32_t* buffers) const noexcept;
    void set_read_buffer(uint32_t src) const noexcept;

    bool is_complete() const noexcept;


    framebuffer(framebuffer&& framebuffer);
    framebuffer& operator=(framebuffer&& framebuffer) noexcept;

    framebuffer(const framebuffer& framebuffer) = delete;
    framebuffer& operator=(const framebuffer& framebuffer) = delete;

private:
    uint32_t m_id = 0;
    mutable bool m_is_complete = false;
};
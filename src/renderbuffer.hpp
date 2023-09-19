#pragma once
#include <cstdint>

#include "nocopyable.hpp"

class renderbuffer : public nocopyable {
public:
    renderbuffer() = default;
    renderbuffer(uint32_t width, uint32_t height, uint32_t internal_format);
    ~renderbuffer();

    void create(uint32_t width, uint32_t height, uint32_t internal_format) noexcept;
    void destroy() noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    uint32_t get_id() const noexcept;
    uint32_t get_width() const noexcept;
    uint32_t get_height() const noexcept;
    uint32_t get_internal_format() const noexcept;


    renderbuffer(renderbuffer&& renderbuffer);
    renderbuffer& operator=(renderbuffer&& renderbuffer) noexcept;

private:
    uint32_t m_id = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_internal_format = 0;
};
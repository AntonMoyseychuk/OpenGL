#pragma once
#include <glad/glad.h>
#include <cstdint>

class index_buffer {
public:
    index_buffer() = default;
    index_buffer(const void* data, size_t count, size_t index_size, decltype(GL_STATIC_DRAW) usage = GL_STATIC_DRAW);

    ~index_buffer();

    void create(const void* data, size_t count, size_t index_size, decltype(GL_STATIC_DRAW) usage = GL_STATIC_DRAW) noexcept;
    void destroy() noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    size_t get_index_count() const noexcept;

    index_buffer(index_buffer&& ibo);
    index_buffer& operator=(index_buffer&& ibo) noexcept;

    index_buffer(const index_buffer& ibo) = delete;
    index_buffer& operator=(const index_buffer& ibo) = delete;


private:
    size_t m_count = 0;
    uint32_t m_id = 0;
    decltype(GL_STATIC_DRAW) m_usage = 0;
};
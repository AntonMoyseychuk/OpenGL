#pragma once
#include <cstdint>
#include <string>

class buffer {
public:
    buffer() = default;
    buffer(uint32_t target, size_t element_count, size_t element_size, uint32_t usage, const void* data);

    ~buffer();

    void create(uint32_t target, size_t element_count, size_t element_size, uint32_t usage, const void* data) noexcept;
    void destroy() noexcept;

    void subdata(uint32_t offset, uint32_t size, const void* data) const noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    size_t get_element_count() const noexcept;
    size_t get_element_size() const noexcept;
    uint32_t get_target() const noexcept;
    uint32_t get_usage() const noexcept;
    uint32_t get_id() const noexcept;

    buffer(buffer&& other) noexcept;
    buffer& operator=(buffer&& other) noexcept;

    buffer(const buffer& other) = delete;
    buffer& operator=(const buffer& other) = delete;

private:
    static const std::string& _target_to_string(uint32_t target) noexcept;

private:
    size_t m_element_count = 0;
    size_t m_element_size = 0;
    uint32_t m_target = 0;
    uint32_t m_usage = 0;
    uint32_t m_id = 0;
};
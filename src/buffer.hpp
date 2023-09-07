#pragma once
#include <cstdint>
#include <string>


struct buffer {
public:
    buffer() = default;
    buffer(int32_t target, size_t size, size_t element_size, int32_t usage, const void* data);

    ~buffer();

    void create(int32_t target, size_t size, size_t element_size, int32_t usage, const void* data) noexcept;
    void destroy() noexcept;

    void subdata(uint32_t offset, size_t size, const void* data) const noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    size_t get_element_count() const noexcept;

    buffer(buffer&& buffer) noexcept;
    buffer& operator=(buffer&& buffer) noexcept;

    buffer(const buffer& buffer) = delete;
    buffer& operator=(const buffer& buffer) = delete;

private:
    static const std::string& _target_to_string(uint32_t target) noexcept;

public:
    size_t size = 0;
    size_t element_size = 0;
    int32_t target = 0;
    int32_t usage = 0;
    uint32_t id = 0;
};
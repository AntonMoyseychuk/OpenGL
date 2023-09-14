#pragma once
#include <array>
#include <string>

class cubemap {
public:
    cubemap() = default;
    cubemap(uint32_t width, uint32_t height, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, 
        const std::array<uint8_t*, 6>& pixels = {});
    cubemap(const std::array<std::string, 6>& faces, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, 
        bool flip_on_load = false);
    ~cubemap();

    void load(const std::array<std::string, 6>& faces, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, 
        bool flip_on_load = false) noexcept;
    void create(uint32_t width, uint32_t height, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, 
        const std::array<uint8_t*, 6>& pixels = {}) noexcept;
    void destroy() noexcept;

    void bind(int32_t unit = -1) const noexcept;
    void unbind() const noexcept;

    void generate_mipmap() const noexcept;
    void set_parameter(uint32_t pname, int32_t param) const noexcept;
    void set_parameter(uint32_t pname, float param) const noexcept;
    void set_parameter(uint32_t pname, const float* params) const noexcept;

    uint32_t get_id() const noexcept;
    uint32_t get_unit() const noexcept;
    uint32_t get_width() const noexcept;
    uint32_t get_height() const noexcept;

    cubemap(cubemap&& cubemap);
    cubemap& operator=(cubemap&& cubemap) noexcept;

    cubemap(const cubemap& cubemap) = delete;
    cubemap& operator=(const cubemap& cubemap) = delete;

private:
    struct data {
        uint32_t id = 0;
        
        uint32_t width = 0;
        uint32_t height = 0;
        
        uint32_t texture_unit = 0;
    };

private:
    data m_data;
};
#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>

class texture {
private:
    struct texture_data {
        uint32_t id = 0;
        uint32_t texture_unit = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channel_count = 0;
    };

public:
    struct config {
        config() = default;
        config(uint32_t type, uint32_t wrap_s, uint32_t wrap_t, uint32_t min_filter, uint32_t mag_filter, bool generate_mipmap);

        uint32_t type;
        uint32_t wrap_s;
        uint32_t wrap_t;
        uint32_t min_filter;
        uint32_t mag_filter;
        bool generate_mipmap;
    };

public:
    texture() = default;
    uint32_t create(const char* filepath, const config& config) const noexcept;

    void activate_unit(uint32_t unit) const noexcept;

    bool bind() const noexcept;
    bool unbind() const noexcept;
    bool is_binded() const noexcept;


private:
    static std::unordered_map<std::string, texture_data> preloaded_textures;

private:
    mutable texture_data m_data;
    mutable config m_config;
    mutable bool m_is_binded = false;
};
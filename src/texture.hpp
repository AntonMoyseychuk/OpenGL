#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>

class texture {
public:
    enum class type { NONE, DIFFUSE, SPECULAR, EMISSION, NORMAL };

    struct config {
        config() = default;
        config(uint32_t target, uint32_t wrap_s, uint32_t wrap_t, uint32_t wrap_r, uint32_t min_filter, uint32_t mag_filter, 
            bool generate_mipmap, type type = type::NONE);

        uint32_t target;
        uint32_t wrap_s;
        uint32_t wrap_t;
        uint32_t wrap_r;
        uint32_t min_filter;
        uint32_t mag_filter;
        type type;
        bool generate_mipmap;
    };

public:
    texture() = default;
    texture(const std::string& filepath, const config& config);

    void create(const std::string& filepath, const config& config) noexcept;

    void bind(uint32_t unit = 0) const noexcept;
    void unbind() const noexcept;

    uint32_t get_id() const noexcept;
    type get_type() const noexcept;

private:
    struct texture_data {
        uint32_t id = 0;
        uint32_t texture_unit = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channel_count = 0;
    };
    
private:
    static std::unordered_map<std::string, texture> preloaded_textures;

private:
    texture_data m_data;
    config m_config;
};
#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>

class texture {
public:
    enum class variety { NONE, DIFFUSE, SPECULAR, EMISSION, NORMAL, HEIGHT };

    struct config {
        config() = default;
        config(
            uint32_t target,
            uint32_t width, uint32_t height, 
            uint32_t internal_format, uint32_t format, uint32_t type,
            uint32_t wrap_s, uint32_t wrap_t, uint32_t wrap_r, uint32_t min_filter, uint32_t mag_filter, 
            bool generate_mipmap = false, variety variety = variety::NONE, uint32_t level = 0
        );

        uint32_t target = 0;
        variety variety = variety::NONE;

        uint32_t texture_unit = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channel_count = 0;

        uint32_t level = 0;
        uint32_t internal_format = 0;
        uint32_t format = 0;
        uint32_t type = 0;

        uint32_t wrap_s = 0;
        uint32_t wrap_t = 0;
        uint32_t wrap_r = 0;
        uint32_t min_filter = 0;
        uint32_t mag_filter = 0;

        bool generate_mipmap = false;
    };

public:
    texture() = default;
    texture(const std::string& filepath, const config& config, bool flip_on_load = true);
    texture(const config& config);
    ~texture();

    void load(const std::string& filepath, const config& config, bool flip_on_load = true) noexcept;
    void create(const config& config) noexcept;
    void destroy() noexcept;

    void bind(uint32_t unit = 0) const noexcept;
    void unbind() const noexcept;

    uint32_t get_id() const noexcept;
    const config& get_config_data() const noexcept;

    texture(texture&& texture);
    texture& operator=(texture&& texture) noexcept;

    texture(const texture& texture) = delete;
    texture& operator=(const texture& texture) = delete;

private:
    void _setup_tex_parametes(const config& config) const noexcept;
    uint32_t _get_channel_correct_format(uint32_t format, uint32_t channel_count) const noexcept;
    
private:
    struct texture_data {
        config config;
        uint32_t id = 0;
    };

private:
    static std::unordered_map<std::string, texture_data> preloaded_textures;

private:
    texture_data m_data;
};
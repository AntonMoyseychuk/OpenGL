#pragma once
#include <glad/glad.h>

#include <string>
#include <cstdint>
#include <unordered_map>

class texture {
public:
    enum class type { NONE, DIFFUSE, SPECULAR, EMISSION, NORMAL };

    struct config {
        config() = default;
        config(decltype(GL_TEXTURE_2D) target, decltype(GL_REPEAT) wrap_s, decltype(GL_REPEAT) wrap_t, decltype(GL_REPEAT) wrap_r, 
            decltype(GL_CLAMP_TO_EDGE) min_filter, decltype(GL_CLAMP_TO_EDGE) mag_filter, bool generate_mipmap, type type = type::NONE);

        decltype(GL_TEXTURE_2D) target;
        decltype(GL_REPEAT) wrap_s;
        decltype(GL_REPEAT) wrap_t;
        decltype(GL_REPEAT) wrap_r;
        decltype(GL_CLAMP_TO_EDGE) min_filter;
        decltype(GL_CLAMP_TO_EDGE) mag_filter;
        type type;
        bool generate_mipmap;
    };

public:
    texture() = default;
    texture(const std::string& filepath, const config& config);
    texture(const config& config, uint32_t width, uint32_t height, decltype(GL_RGB) format);

    void load(const std::string& filepath, const config& config) noexcept;
    void create(const config& config, uint32_t width, uint32_t height, decltype(GL_RGB) format) noexcept;
    void destroy() noexcept;

    void bind(uint32_t unit = 0) const noexcept;
    void unbind() const noexcept;

    uint32_t get_id() const noexcept;
    type get_type() const noexcept;

private:
    void _setup_tex_parametes(const config& config) const noexcept;

private:
    struct texture_data {
        uint32_t id = 0;
        uint32_t texture_unit = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channel_count = 0;

        decltype(GL_TEXTURE_2D) target = GL_FALSE;
        type type = type::NONE;
    };
    
private:
    static std::unordered_map<std::string, texture_data> preloaded_textures;

private:
    texture_data m_data;
};
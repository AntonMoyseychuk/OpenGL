#include "texture.hpp"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "debug.hpp"
#include "log.hpp"

std::unordered_map<std::string, texture::texture_data> texture::preloaded_textures;

texture::texture(const std::string &filepath, const config &config, bool flip_on_load) {
    load(filepath, config, flip_on_load);
}

texture::texture(const config &config) {
    create(config);
}

texture::~texture() {
    destroy();
}

void texture::load(const std::string &filepath, const config &config, bool flip_on_load) noexcept {
    if (m_data.id != 0) {
        LOG_WARN("texture warning", "texture reloading (prev id = " + std::to_string(m_data.id) + ")");
        destroy();
    }

    if (const auto& t = preloaded_textures.find(filepath); t != preloaded_textures.cend()) {
        m_data = t->second;
        return;
    }

    m_data.config = config;
    
    stbi_set_flip_vertically_on_load(flip_on_load);
    uint8_t* texture_data = stbi_load(filepath.c_str(), (int*)&m_data.config.width, (int*)&m_data.config.height, (int*)&m_data.config.channel_count, 0);
    
    ASSERT(texture_data != nullptr, "texture error", "couldn't load texture \"" + filepath + "\"");

    OGL_CALL(glGenTextures(1, &m_data.id));
    bind();

    m_data.config.internal_format = _get_channel_correct_format(m_data.config.internal_format, m_data.config.channel_count);
    m_data.config.format = _get_channel_correct_format(m_data.config.format, m_data.config.channel_count);
    OGL_CALL(glTexImage2D(m_data.config.target, m_data.config.level, m_data.config.internal_format, m_data.config.width, m_data.config.height, 0, m_data.config.format, m_data.config.type, texture_data));
    if (m_data.config.generate_mipmap) {
        OGL_CALL(glGenerateMipmap(m_data.config.target));
    }

    _setup_tex_parametes(config);

    stbi_image_free(texture_data);

    preloaded_textures[filepath] = m_data;

    unbind();
}

void texture::create(const config &config) noexcept {
    if (m_data.id != 0) {
        LOG_WARN("texture warning", "texture recreation (prev id = " + std::to_string(m_data.id) + ")");
        destroy();
    }

    m_data.config = config;

    OGL_CALL(glGenTextures(1, &m_data.id));
    bind();

    OGL_CALL(glTexImage2D(m_data.config.target, m_data.config.level, m_data.config.internal_format, m_data.config.width, m_data.config.height, 0, m_data.config.format, m_data.config.type, nullptr));
    if (m_data.config.generate_mipmap) {
        OGL_CALL(glGenerateMipmap(m_data.config.target));
    }

    _setup_tex_parametes(m_data.config);

    unbind();
}

void texture::destroy() noexcept {
    OGL_CALL(glDeleteTextures(1, &m_data.id));
    memset(&m_data, 0, sizeof(m_data));
}

void texture::bind(uint32_t unit) const noexcept {
#ifdef _DEBUG
    int32_t max_units_count;
    OGL_CALL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_units_count));
    ASSERT(unit < max_units_count, "texture error", "unit value is greater than GL_MAX_TEXTURE_UNITS");
#endif

    const_cast<texture*>(this)->m_data.config.texture_unit = GL_TEXTURE0 + unit;
    OGL_CALL(glActiveTexture(m_data.config.texture_unit));

    OGL_CALL(glBindTexture(m_data.config.target, m_data.id));
}

void texture::unbind() const noexcept {
    OGL_CALL(glBindTexture(m_data.config.target, 0));
}

uint32_t texture::get_id() const noexcept {
    return m_data.id;
}

const texture::config &texture::get_config_data() const noexcept {
    return m_data.config;
}

texture::texture(texture &&texture)
    : m_data(texture.m_data)
{
    memset(&texture, 0, sizeof(texture));
}

texture &texture::operator=(texture &&texture) noexcept {
    m_data = texture.m_data;
    memset(&texture, 0, sizeof(texture));

    return *this;
}

void texture::_setup_tex_parametes(const config& config) const noexcept {
    ASSERT(m_data.config.target == config.target, "texture error", "m_data.config.target != config.target");

    if (config.wrap_s != GL_FALSE) {
        OGL_CALL(glTexParameteri(m_data.config.target, GL_TEXTURE_WRAP_S, config.wrap_s));
    }
    if (config.wrap_t != GL_FALSE) {
        OGL_CALL(glTexParameteri(m_data.config.target, GL_TEXTURE_WRAP_T, config.wrap_t));
    }
    if (config.wrap_r != GL_FALSE) {
        OGL_CALL(glTexParameteri(m_data.config.target, GL_TEXTURE_WRAP_R, config.wrap_r));
    }
    if (config.min_filter != GL_FALSE) {
        OGL_CALL(glTexParameteri(m_data.config.target, GL_TEXTURE_MIN_FILTER, config.min_filter));
    }
    if (config.mag_filter != GL_FALSE) {
        OGL_CALL(glTexParameteri(m_data.config.target, GL_TEXTURE_MAG_FILTER, config.mag_filter));
    }
}

uint32_t texture::_get_channel_correct_format(uint32_t format, uint32_t channel_count) const noexcept {
    switch (format) {
    case GL_RGB:
    case GL_RGBA:
        return channel_count == 3 ? GL_RGB : GL_RGBA;

    case GL_SRGB:
    case GL_SRGB_ALPHA:
        return channel_count == 3 ? GL_SRGB : GL_SRGB_ALPHA;
    }

    ASSERT(false, "texture error", "Unknown texture file format");
    return 0;
}

texture::config::config(
    uint32_t target, 
    uint32_t width, uint32_t height, 
    uint32_t internal_format, uint32_t format, uint32_t type, 
    uint32_t wrap_s, uint32_t wrap_t, uint32_t wrap_r, uint32_t min_filter, uint32_t mag_filter, 
    bool generate_mipmap, texture::variety variety, uint32_t level
) : target(target), variety(variety), width(width), height(height), level(level), internal_format(internal_format), format(format), type(type), 
    wrap_s(wrap_s), wrap_t(wrap_t), wrap_r(wrap_r), min_filter(min_filter), mag_filter(mag_filter), generate_mipmap(generate_mipmap)
{
}

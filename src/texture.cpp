#include "texture.hpp"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "debug.hpp"
#include "log.hpp"

std::unordered_map<std::string, texture::texture_data> texture::preloaded_textures;

texture::texture(const std::string &filepath, const config &config) {
    load(filepath, config);
}

texture::texture(uint32_t width, uint32_t height, uint32_t format, const config &config) {
    create(width, height, format, config);
}

texture::~texture() {
    destroy();
}

void texture::load(const std::string &filepath, const config &config) noexcept {
    if (m_data.id != 0) {
        LOG_WARN("texture warning", "texture reloading (prev id = " + std::to_string(m_data.id) + ")");
        destroy();
    }

    if (const auto& t = preloaded_textures.find(filepath); t != preloaded_textures.cend()) {
        m_data = t->second;
        return;
    }

    m_data.target = config.target;
    m_data.type = config.type;
    
    stbi_set_flip_vertically_on_load(true);
    uint8_t* texture_data = stbi_load(filepath.c_str(), (int*)&m_data.width, (int*)&m_data.height, (int*)&m_data.channel_count, 0);
    
    ASSERT(texture_data != nullptr, "texture error", "couldn't load texture \"" + filepath + "\"");

    OGL_CALL(glGenTextures(1, &m_data.id));
    bind();

    const uint32_t format = m_data.channel_count == 3 ? GL_RGB : GL_RGBA;
    OGL_CALL(glTexImage2D(m_data.target, 0, format, m_data.width, m_data.height, 0, format, GL_UNSIGNED_BYTE, texture_data));
    if (config.generate_mipmap) {
        OGL_CALL(glGenerateMipmap(m_data.target));
    }

    _setup_tex_parametes(config);

    stbi_image_free(texture_data);

    preloaded_textures[filepath] = m_data;

    unbind();
}

void texture::create(uint32_t width, uint32_t height, uint32_t format, const config &config) noexcept {
    if (m_data.id != 0) {
        LOG_WARN("texture warning", "texture recreation (prev id = " + std::to_string(m_data.id) + ")");
        destroy();
    }

    m_data.width = width;
    m_data.height = height;
    m_data.target = config.target;
    m_data.type = config.type;

    OGL_CALL(glGenTextures(1, &m_data.id));
    bind();

    OGL_CALL(glTexImage2D(m_data.target, 0, format, m_data.width, m_data.height, 0, format, GL_UNSIGNED_BYTE, nullptr));
    if (config.generate_mipmap) {
        OGL_CALL(glGenerateMipmap(m_data.target));
    }

    _setup_tex_parametes(config);

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

    const_cast<texture*>(this)->m_data.texture_unit = GL_TEXTURE0 + unit;
    OGL_CALL(glActiveTexture(m_data.texture_unit));

    OGL_CALL(glBindTexture(m_data.target, m_data.id));
}

void texture::unbind() const noexcept {
    OGL_CALL(glBindTexture(m_data.target, 0));
}

uint32_t texture::get_id() const noexcept {
    return m_data.id;
}

texture::type texture::get_type() const noexcept {
    return m_data.type;
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
    if (config.wrap_s != GL_FALSE) {
        OGL_CALL(glTexParameteri(m_data.target, GL_TEXTURE_WRAP_S, config.wrap_s));
    }
    if (config.wrap_t != GL_FALSE) {
        OGL_CALL(glTexParameteri(m_data.target, GL_TEXTURE_WRAP_T, config.wrap_t));
    }
    if (config.wrap_r != GL_FALSE) {
        OGL_CALL(glTexParameteri(m_data.target, GL_TEXTURE_WRAP_R, config.wrap_r));
    }
    if (config.min_filter != GL_FALSE) {
        OGL_CALL(glTexParameteri(m_data.target, GL_TEXTURE_MIN_FILTER, config.min_filter));
    }
    if (config.mag_filter != GL_FALSE) {
        OGL_CALL(glTexParameteri(m_data.target, GL_TEXTURE_MAG_FILTER, config.mag_filter));
    }
}

texture::config::config(uint32_t target, uint32_t wrap_s, uint32_t wrap_t, uint32_t wrap_r, uint32_t min_filter, uint32_t mag_filter, 
    bool generate_mipmap, texture::type type) 
        : target(target), wrap_s(wrap_s), wrap_t(wrap_t), wrap_r(wrap_r), min_filter(min_filter), mag_filter(mag_filter), 
            type(type), generate_mipmap(generate_mipmap)
{
}

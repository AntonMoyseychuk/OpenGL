#include "texture.hpp"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "debug.hpp"
#include "log.hpp"

std::unordered_map<std::string, texture> texture::preloaded_textures;

texture::texture(const std::string &filepath, const config &config) {
    create(filepath, config);
}

void texture::create(const std::string &filepath, const config &config) const noexcept {
    texture* self = const_cast<texture*>(this);
    
    if (const auto& t = preloaded_textures.find(filepath); t != preloaded_textures.cend()) {
        self->m_config = t->second.m_config;
        self->m_data = t->second.m_data;
        return;
    }

    self->m_config = config;
    
    stbi_set_flip_vertically_on_load(true);
    uint8_t* texture_data = stbi_load(filepath.c_str(), (int*)&m_data.width, (int*)&m_data.height, (int*)&m_data.channel_count, 0);
    
    ASSERT(texture_data != nullptr, "texture error", "couldn't load texture \"" + filepath + "\"");

    OGL_CALL(glGenTextures(1, &self->m_data.id));
    bind();

    const uint32_t format = m_data.channel_count == 3 ? GL_RGB : GL_RGBA;
    OGL_CALL(glTexImage2D(m_config.target, 0, format, m_data.width, m_data.height, 0, format, GL_UNSIGNED_BYTE, texture_data));
    if (config.generate_mipmap) {
        OGL_CALL(glGenerateMipmap(m_config.target));
    }

    OGL_CALL(glTexParameteri(m_config.target, GL_TEXTURE_WRAP_S, m_config.wrap_s));
    OGL_CALL(glTexParameteri(m_config.target, GL_TEXTURE_WRAP_T, m_config.wrap_t));
    OGL_CALL(glTexParameteri(m_config.target, GL_TEXTURE_MIN_FILTER, m_config.min_filter));
    OGL_CALL(glTexParameteri(m_config.target, GL_TEXTURE_MAG_FILTER, m_config.mag_filter));

    stbi_image_free(texture_data);

    preloaded_textures[filepath] = *self;
}

void texture::activate_unit(uint32_t unit) const noexcept {
#ifdef _DEBUG
    int32_t max_units_count;
    OGL_CALL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_units_count));
    ASSERT(unit < max_units_count, "texture error", "unit value is greater than GL_MAX_TEXTURE_UNITS");
#endif

    const_cast<texture*>(this)->m_data.texture_unit = GL_TEXTURE0 + unit;
    OGL_CALL(glActiveTexture(m_data.texture_unit));
    bind();
}

void texture::bind() const noexcept {
    OGL_CALL(glBindTexture(m_config.target, m_data.id));
}

void texture::unbind() const noexcept {
    OGL_CALL(glBindTexture(m_config.target, 0));
}

uint32_t texture::get_id() const noexcept {
    return m_data.id;
}

texture::type texture::get_type() const noexcept {
    return m_config.type;
}

texture::config::config(uint32_t target, uint32_t wrap_s, uint32_t wrap_t, uint32_t min_filter, uint32_t mag_filter, bool generate_mipmap, texture::type type)
    : target(target), wrap_s(wrap_s), wrap_t(wrap_t), min_filter(min_filter), mag_filter(mag_filter), type(type), generate_mipmap(generate_mipmap)
{
}

#include "texture.hpp"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "assert.hpp"

std::unordered_map<std::string, texture::texture_data> texture::preloaded_textures;

uint32_t texture::create(const char *filepath, const config& config) const noexcept {
    if (preloaded_textures.find(filepath) != preloaded_textures.cend()) {
        return preloaded_textures.at(filepath).id;
    }

    m_config = config;
    
    stbi_set_flip_vertically_on_load(true);
    uint8_t* texture_data = stbi_load(filepath, (int*)&m_data.width, (int*)&m_data.height, (int*)&m_data.channel_count, 0);
    if (texture_data == nullptr) {
        spdlog::error("texture error: couldn't load texture from {}", filepath);
        return 0;
    }

    glGenTextures(1, &m_data.id);
    if (bind(); !is_binded()) {
        spdlog::error("texture error: texture binding failed");
        stbi_image_free(texture_data);    
        return 0;
    }

    const uint32_t format = m_data.channel_count == 3 ? GL_RGB : GL_RGBA;
    glTexImage2D(m_config.type, 0, format, m_data.width, m_data.height, 0, format, GL_UNSIGNED_BYTE, texture_data);
    if (config.generate_mipmap) {
        glGenerateMipmap(m_config.type);
    }

    glTextureParameteri(m_config.type, GL_TEXTURE_WRAP_S, m_config.wrap_s);
    glTextureParameteri(m_config.type, GL_TEXTURE_WRAP_T, m_config.wrap_t);
    glTextureParameteri(m_config.type, GL_TEXTURE_MIN_FILTER, m_config.min_filter);
    glTextureParameteri(m_config.type, GL_TEXTURE_MAG_FILTER, m_config.mag_filter);

    stbi_image_free(texture_data);

    preloaded_textures[filepath] = m_data;

    return m_data.id;
}

void texture::activate_unit(uint32_t unit) const noexcept {
#ifdef _DEBUG
    int32_t max_units_count;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_units_count);
    ASSERT(unit < max_units_count, "texture error", "unit value is greater than GL_MAX_TEXTURE_UNITS");
#endif

    m_data.texture_unit = GL_TEXTURE0 + unit;
    glActiveTexture(m_data.texture_unit);
    bind();
}

bool texture::bind() const noexcept {
    const bool was_binded = m_is_binded;

    glBindTexture(m_config.type, m_data.id);
    m_is_binded = (m_data.id != 0);

    return was_binded;
}

bool texture::unbind() const noexcept {
    const bool was_binded = m_is_binded;

    glBindTexture(m_config.type, 0);
    m_is_binded = false;

    return was_binded;
}

bool texture::is_binded() const noexcept {
    return m_is_binded;
}

texture::config::config(uint32_t type, uint32_t wrap_s, uint32_t wrap_t, uint32_t min_filter, uint32_t mag_filter, bool generate_mipmap)
    : type(type), wrap_s(wrap_s), wrap_t(wrap_t), min_filter(min_filter), mag_filter(mag_filter), generate_mipmap(generate_mipmap)
{
}

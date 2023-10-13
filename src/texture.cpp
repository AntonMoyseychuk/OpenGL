#include "texture.hpp"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "debug.hpp"
#include "log.hpp"

std::unordered_map<std::string, texture_2d::data> texture_2d::preloaded_textures;

texture_2d::texture_2d(const std::string &filepath, bool flip_on_load, bool use_gamma, variety variety) {
    load(filepath, flip_on_load, use_gamma, variety);
}

texture_2d::texture_2d(uint32_t width, uint32_t height, int32_t level, 
    int32_t internal_format, int32_t format, int32_t type, void *pixels, variety variety
) {
    create(width, height, level, internal_format, format, type, pixels, variety);
}

texture_2d::~texture_2d() {
    // NOTE: I realized that I need something like texture manager because mesh contains textures and deletes them in destructor
    // but they still remains in preloaded_textures 'cache'. And if I for example recreate terain mesh, it gets invalid texture data from 'cache'.
    // destroy();
}

void texture_2d::load(const std::string &filepath, bool flip_on_load, bool use_gamma, variety variety) noexcept {
    if (m_data.id != 0) {
        LOG_WARN("texture warning", "texture reloading (prev id = " + std::to_string(m_data.id) + ")");
        destroy();
    }

    if (const auto& t = preloaded_textures.find(filepath); t != preloaded_textures.cend()) {
        m_data = t->second;
        return;
    }
    
    stbi_set_flip_vertically_on_load(flip_on_load);

    int channel_count = 0;
    uint8_t* texture_data = stbi_load(filepath.c_str(), (int*)&m_data.width, (int*)&m_data.height, &channel_count, 0);  
    ASSERT(texture_data != nullptr, "texture error", "couldn't load texture \"" + filepath + "\"");

    const int32_t format = _get_gl_format(channel_count, false);
    const int32_t internal_format = _get_gl_format(channel_count, use_gamma);

    create(m_data.width, m_data.height, 0, internal_format, format, GL_UNSIGNED_BYTE, texture_data, variety);
    
    stbi_image_free(texture_data);

    preloaded_textures[filepath] = m_data;
}

void texture_2d::create(uint32_t width, uint32_t height, int32_t level, 
    int32_t internal_format, int32_t format, int32_t type, void* pixels, variety variety
) noexcept {
    if (m_data.id != 0) {
        LOG_WARN("texture warning", "texture recreation (prev id = " + std::to_string(m_data.id) + ")");
        destroy();
    }

    m_data.width = width;
    m_data.height = height;
    m_data.variety = variety;

    OGL_CALL(glGenTextures(1, &m_data.id));
    bind();

    OGL_CALL(glTexImage2D(GL_TEXTURE_2D, level, internal_format, m_data.width, m_data.height, 0, format, type, pixels));
}

void texture_2d::destroy() noexcept {
    OGL_CALL(glDeleteTextures(1, &m_data.id));
    memset(&m_data, 0, sizeof(m_data));
}

void texture_2d::generate_mipmap() const noexcept {
    bind();
    OGL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
}

void texture_2d::set_parameter(uint32_t pname, int32_t param) const noexcept {
    bind();
    OGL_CALL(glTexParameteri(GL_TEXTURE_2D, pname, param));
}

void texture_2d::set_parameter(uint32_t pname, float param) const noexcept {
    bind();
    OGL_CALL(glTexParameterf(GL_TEXTURE_2D, pname, param));
}

void texture_2d::set_parameter(uint32_t pname, const float* params) const noexcept {
    bind();
    OGL_CALL(glTexParameterfv(GL_TEXTURE_2D, pname, params));
}

void texture_2d::bind(int32_t unit) const noexcept {
#ifdef _DEBUG
    int32_t max_units_count;
    OGL_CALL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_units_count));
    ASSERT(unit < max_units_count, "texture error", "unit value is greater than GL_MAX_TEXTURE_UNITS");
#endif

    if (unit >= 0) {
        const_cast<texture_2d*>(this)->m_data.texture_unit = unit;
        OGL_CALL(glActiveTexture(GL_TEXTURE0 + unit));
    }

    OGL_CALL(glBindTexture(GL_TEXTURE_2D, m_data.id));
}

void texture_2d::unbind() const noexcept {
    OGL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
}

uint32_t texture_2d::get_id() const noexcept {
    return m_data.id;
}

uint32_t texture_2d::get_unit() const noexcept {
    return m_data.texture_unit;
}

uint32_t texture_2d::get_width() const noexcept {
    return m_data.width;
}

uint32_t texture_2d::get_height() const noexcept {
    return m_data.height;
}

texture_2d::variety texture_2d::get_variety() const noexcept {
    return m_data.variety;
}

texture_2d::texture_2d(texture_2d &&texture)
    : m_data(texture.m_data)
{
    if (this != &texture) {
        memset(&texture, 0, sizeof(texture));
    }
}

texture_2d &texture_2d::operator=(texture_2d &&texture) noexcept {
    if (this != &texture) {
        m_data = texture.m_data;
        memset(&texture, 0, sizeof(texture));
    }

    return *this;
}

int32_t texture_2d::_get_gl_format(int32_t channel_count, bool use_gamma) const noexcept {
    switch (channel_count) {
    case 1:
        return GL_RED;

    case 3:
        return use_gamma ? GL_SRGB : GL_RGB;

    case 4:
        return use_gamma ? GL_SRGB_ALPHA : GL_RGBA;
    
    default:
        ASSERT(false, "texture_2d", "invalid channel count");
        return 0;
    }
}

#include "texture.hpp"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "debug.hpp"
#include "log.hpp"

std::unordered_map<std::string, texture_2d::data> texture_2d::preloaded_textures;

texture_2d::texture_2d(const std::string &filepath, int32_t target, int32_t level, 
    int32_t internal_format, int32_t format, int32_t type, bool flip_on_load, variety variety
) {
    load(filepath, target, level, internal_format, format, type, flip_on_load, variety);
}

texture_2d::texture_2d(uint32_t width, uint32_t height, int32_t target, int32_t level, 
    int32_t internal_format, int32_t format, int32_t type, void *pixels, variety variety
) {
    create(width, height, target, level, internal_format, format, type, pixels, variety);
}

texture_2d::~texture_2d() {
    destroy();
}

void texture_2d::load(const std::string &filepath, int32_t target, int32_t level, 
    int32_t internal_format, int32_t format, int32_t type, bool flip_on_load, variety variety
) noexcept {
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

    create(m_data.width, m_data.height, target, level, internal_format, format, type, texture_data);
    
    stbi_image_free(texture_data);

    preloaded_textures[filepath] = m_data;
}

void texture_2d::create(uint32_t width, uint32_t height, int32_t target, int32_t level, 
    int32_t internal_format, int32_t format, int32_t type, void* pixels, variety variety
) noexcept {
    if (m_data.id != 0) {
        LOG_WARN("texture warning", "texture recreation (prev id = " + std::to_string(m_data.id) + ")");
        destroy();
    }

    m_data.target = target;
    m_data.width = width;
    m_data.height = height;
    m_data.variety = variety;

    OGL_CALL(glGenTextures(1, &m_data.id));
    bind();

    OGL_CALL(glTexImage2D(m_data.target, level, internal_format, m_data.width, m_data.height, 0, format, type, pixels));
}

void texture_2d::destroy() noexcept {
    OGL_CALL(glDeleteTextures(1, &m_data.id));
    memset(&m_data, 0, sizeof(m_data));
}

void texture_2d::generate_mipmap() const noexcept {
    OGL_CALL(glGenerateMipmap(m_data.target));
}

void texture_2d::set_tex_parameter(uint32_t pname, int32_t param) const noexcept {
    OGL_CALL(glTexParameteri(m_data.target, pname, param));
}

void texture_2d::set_tex_parameter(uint32_t pname, float param) const noexcept {
    OGL_CALL(glTexParameterf(m_data.target, pname, param));
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

    OGL_CALL(glBindTexture(m_data.target, m_data.id));
}

void texture_2d::unbind() const noexcept {
    OGL_CALL(glBindTexture(m_data.target, 0));
}

uint32_t texture_2d::get_id() const noexcept {
    return m_data.id;
}

uint32_t texture_2d::get_unit() const noexcept {
    return m_data.texture_unit;
}

uint32_t texture_2d::get_target() const noexcept {
    return m_data.target;
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
    memset(&texture, 0, sizeof(texture));
}

texture_2d &texture_2d::operator=(texture_2d &&texture) noexcept {
    m_data = texture.m_data;
    memset(&texture, 0, sizeof(texture));

    return *this;
}
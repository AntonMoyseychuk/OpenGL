#include "cubemap.hpp"

#include "debug.hpp"

#include <glad/glad.h>
#include <stb/stb_image.h>

cubemap::~cubemap() {
    destroy();
}

cubemap::cubemap(const config &config) {
    create(config);
}

cubemap::cubemap(const std::vector<std::string> &faces, const config &config, bool flip_on_load) {
    load(faces, config, flip_on_load);
}

void cubemap::load(const std::vector<std::string> &faces, const config& config, bool flip_on_load) noexcept {
    if (m_data.id != 0) {
        LOG_WARN("cubemap warning", "cubemap recreation (prev id = " + std::to_string(m_data.id) + ")");
        destroy();
    }

    m_data.config = config;

    OGL_CALL(glGenTextures(1, &m_data.id));
    bind();

    ASSERT(faces.size() == 6, "cubemap creation error", "faces count is not equal 6");

    stbi_set_flip_vertically_on_load(flip_on_load);
    for (size_t i = 0; i < faces.size(); ++i) {
        uint8_t* texture_data = stbi_load(faces[i].c_str(), (int*)&m_data.config.face_width, (int*)&m_data.config.face_height, (int*)&m_data.channel_count, 0);
    
        ASSERT(texture_data != nullptr, "cubemap creation error", "couldn't load texture \"" + faces[i] + "\"");

        OGL_CALL(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_data.config.internal_format, 
            m_data.config.face_width, m_data.config.face_width, 0, m_data.config.format, m_data.config.type, texture_data));

        stbi_image_free(texture_data);
    }
    _setup_tex_parametes(config);
}

void cubemap::create(const config &config) noexcept {
    if (m_data.id != 0) {
        LOG_WARN("cubemap warning", "cubemap recreation (prev id = " + std::to_string(m_data.id) + ")");
        destroy();
    }

    m_data.config = config;

    OGL_CALL(glGenTextures(1, &m_data.id));
    bind();

    for (size_t i = 0; i < 6; ++i) {
        OGL_CALL(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_data.config.internal_format, 
            m_data.config.face_width, m_data.config.face_width, 0, m_data.config.format, m_data.config.type, nullptr));
    }
    _setup_tex_parametes(config);
}

void cubemap::destroy() noexcept {
    OGL_CALL(glDeleteTextures(1, &m_data.id));
    m_data.id = 0;
}

void cubemap::bind(uint32_t unit) const noexcept {
#ifdef _DEBUG
    int32_t max_units_count;
    OGL_CALL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_units_count));
    ASSERT(unit < max_units_count, "texture error", "unit value is greater than GL_MAX_TEXTURE_UNITS");
#endif
    const_cast<cubemap*>(this)->m_data.texture_unit = unit;
    OGL_CALL(glActiveTexture(GL_TEXTURE0 + unit));

    OGL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_data.id));
}

void cubemap::unbind() const noexcept {
    OGL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
}

uint32_t cubemap::get_id() const noexcept {
    return m_data.id;
}

uint32_t cubemap::get_unit() const noexcept {
    return m_data.texture_unit;
}

const cubemap::config &cubemap::get_config_data() const noexcept {
    return m_data.config;
}

cubemap::cubemap(cubemap &&cubemap)
    : m_data(cubemap.m_data)
{
    memset(&cubemap.m_data, 0, sizeof(cubemap.m_data));
}

cubemap &cubemap::operator=(cubemap &&cubemap) noexcept {
    m_data = cubemap.m_data;
    memset(&cubemap.m_data, 0, sizeof(cubemap.m_data));
    
    return *this;
}

void cubemap::_setup_tex_parametes(const config &config) const noexcept {
    if (config.wrap_s != GL_FALSE) {
        OGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, config.wrap_s));
    }
    if (config.wrap_t != GL_FALSE) {
        OGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, config.wrap_t));
    }
    if (config.wrap_r != GL_FALSE) {
        OGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, config.wrap_r));
    }
    if (config.min_filter != GL_FALSE) {
        OGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, config.min_filter));
    }
    if (config.mag_filter != GL_FALSE) {
        OGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, config.mag_filter));
    }
}

uint32_t cubemap::_get_channel_correct_format(uint32_t format, uint32_t channel_count) const noexcept {
    switch (format) {
    case GL_RGB:
    case GL_RGBA:
        return channel_count == 3 ? GL_RGB : GL_RGBA;

    case GL_SRGB:
    case GL_SRGB_ALPHA:
        return channel_count == 3 ? GL_SRGB : GL_SRGB_ALPHA;
    }

    ASSERT(false, "cubemap error", "Unknown cubemap format");
    return 0;
}

cubemap::config::config(
    uint32_t face_width, uint32_t face_height, 
    uint32_t internal_format, uint32_t format, uint32_t type, 
    uint32_t wrap_s, uint32_t wrap_t, uint32_t wrap_r, uint32_t min_filter, uint32_t mag_filter
) : face_width(face_width), face_height(face_height), internal_format(internal_format), format(format), type(type),
    wrap_s(wrap_s), wrap_t(wrap_t), wrap_r(wrap_r), min_filter(min_filter), mag_filter(mag_filter)
{
}

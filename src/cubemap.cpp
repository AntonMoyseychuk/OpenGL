#include "cubemap.hpp"

#include "texture.hpp"
#include "debug.hpp"

#include <glad/glad.h>
#include <stb/stb_image.h>

cubemap::~cubemap() {
    destroy();
}

cubemap::cubemap(const std::vector<std::string> &faces) {
    create(faces);
}

void cubemap::create(const std::vector<std::string> &faces) noexcept {
    if (m_id != 0) {
        LOG_WARN("cubemap warning", "cubemap recreation (prev id = " + std::to_string(m_id) + ")");
        destroy();
    }

    OGL_CALL(glGenTextures(1, &m_id));
    bind();

    ASSERT(faces.size() == 6, "cubemap creation error", "faces count is not equal 6");

    for (size_t i = 0; i < faces.size(); ++i) {
        int32_t width, height, channel_count;

        stbi_set_flip_vertically_on_load(false);
        uint8_t* texture_data = stbi_load(faces[i].c_str(), &width, &height, &channel_count, 0);
    
        ASSERT(texture_data != nullptr, "cubemap creation error", "couldn't load texture \"" + faces[i] + "\"");

        OGL_CALL(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_data));

        stbi_image_free(texture_data);
    }

    OGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    OGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    OGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
    OGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    OGL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    unbind();
}

void cubemap::destroy() noexcept {
    OGL_CALL(glDeleteTextures(1, &m_id));
    m_id = 0;
}

void cubemap::bind(uint32_t unit) const noexcept {
#ifdef _DEBUG
    int32_t max_units_count;
    OGL_CALL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_units_count));
    ASSERT(unit < max_units_count, "texture error", "unit value is greater than GL_MAX_TEXTURE_UNITS");
#endif

    OGL_CALL(glActiveTexture(GL_TEXTURE0 + unit));
    OGL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_id));
}

void cubemap::unbind() const noexcept {
    OGL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
}

cubemap::cubemap(cubemap &&other)
    : m_id(other.m_id)
{
    other.m_id = 0;
}

cubemap &cubemap::operator=(cubemap &&other) noexcept {
    m_id = other.m_id;
    other.m_id = 0;
    
    return *this;
}

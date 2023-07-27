#include "cubemap.hpp"

#include "texture.hpp"
#include "debug.hpp"

#include <glad/glad.h>
#include <stb/stb_image.h>

cubemap::cubemap(const std::vector<std::string> &faces, bool flip_on_load) {
    this->create(faces, flip_on_load);
}

void cubemap::create(const std::vector<std::string> &faces, bool flip_on_load) noexcept {
    OGL_CALL(glGenTextures(1, &m_id));
    this->bind();

    ASSERT(faces.size() == 6, "cubemap creation error", "faces count is not equal 6");

    for (size_t i = 0; i < faces.size(); ++i) {
        int32_t width, height, channel_count;

        stbi_set_flip_vertically_on_load(flip_on_load);
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

    this->unbind();
}

void cubemap::bind() const noexcept {
    OGL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_id));
}

void cubemap::unbind() const noexcept {
    OGL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
}

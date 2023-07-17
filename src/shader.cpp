#include "shader.hpp"

#include <fstream>
#include <sstream>

std::unordered_map<std::string, uint32_t> shader::precompiled_shaders;

uint32_t shader::create(const char *vs_filepath, const char *fs_filepath) const noexcept {
    int32_t success;
    
    uint32_t vs_id = _create_shader(GL_VERTEX_SHADER, vs_filepath);
    glGetShaderiv(vs_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        int32_t error_msg_len = 0;
        glGetShaderiv(vs_id, GL_INFO_LOG_LENGTH, &error_msg_len);

        char* error_msg = (char*)alloca(error_msg_len);
        glGetShaderInfoLog(vs_id, error_msg_len, nullptr, error_msg);
        spdlog::error("{} shader compilation error: {}", vs_filepath, error_msg);

        glDeleteShader(vs_id);
        return false;
    }

    uint32_t fs_id = _create_shader(GL_FRAGMENT_SHADER, fs_filepath);
    glGetShaderiv(fs_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        int32_t error_msg_len = 0;
        glGetShaderiv(fs_id, GL_INFO_LOG_LENGTH, &error_msg_len);

        char* error_msg = (char*)alloca(error_msg_len);
        glGetShaderInfoLog(fs_id, error_msg_len, nullptr, error_msg);
        spdlog::error("{} shader compilation error: {}", fs_filepath, error_msg);

        glDeleteShader(fs_id);
        return false;
    }

    m_program_id = _create_shader_program(vs_id, fs_id);

    return m_program_id;
}

uint32_t shader::get_id() const noexcept {
    return m_program_id;
}

bool shader::bind() const noexcept {
    const bool was_binded = m_is_binded;

    glUseProgram(m_program_id);
    m_is_binded = (m_program_id != 0);

    return was_binded;
}

bool shader::unbind() const noexcept {
    const bool was_binded = m_is_binded;

    glUseProgram(0);
    m_is_binded = false;

    return was_binded;
}

bool shader::is_binded() const noexcept {
    return m_is_binded;
}

void shader::uniform(const char *name, bool uniform) const noexcept {
    _set_uniform(glUniform1ui, name, uniform);
}

void shader::uniform(const char *name, float uniform) const noexcept {
    _set_uniform(glUniform1f, name, uniform);
}

void shader::uniform(const char *name, double uniform) const noexcept {
    _set_uniform(glUniform1d, name, uniform);
}

void shader::uniform(const char *name, int32_t uniform) const noexcept {
    _set_uniform(glUniform1i, name, uniform);
}

void shader::uniform(const char *name, uint32_t uniform) const noexcept {
    _set_uniform(glUniform1ui, name, uniform);
}

void shader::uniform(const char *name, const glm::vec3& uniform) const noexcept {
    _set_uniform(glUniform3fv, name, 1, glm::value_ptr(uniform));
}

void shader::uniform(const char *name, const glm::vec4& uniform) const noexcept {
    _set_uniform(glUniform4fv, name, 1, glm::value_ptr(uniform));
}

void shader::uniform(const char *name, const glm::mat4& uniform) const noexcept {
    _set_uniform(glUniformMatrix4fv, name, 1, false, glm::value_ptr(uniform));
}

bool shader::operator==(const shader &shader) const noexcept {
    return m_program_id == shader.m_program_id;
}

bool shader::operator!=(const shader &shader) const noexcept {
    return !this->operator==(shader);
}

std::string shader::_read_shader_data_from_file(const char *filepath) noexcept {
    std::ifstream file(filepath);

    if (file.is_open()) {
        std::stringstream shader_stream;
        shader_stream << file.rdbuf();

        return shader_stream.str();
    }

    spdlog::error("invalid shader source filepath: {}", filepath);
    return "";
}

uint32_t shader::_compile_shader(GLenum shader_type, const char *source) noexcept {
    const uint32_t id = glCreateShader(shader_type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    return id;
}

uint32_t shader::_create_shader(GLenum shader_type, const char *filepath) noexcept {
    uint32_t id = 0;
    if (precompiled_shaders.find(filepath) != precompiled_shaders.cend()) {
        id = precompiled_shaders.at(filepath);
    } else {
        id = _compile_shader(shader_type, _read_shader_data_from_file(filepath).c_str());
        if (id != 0) {
            precompiled_shaders[filepath] = id;
        }
    }

    return id;
}

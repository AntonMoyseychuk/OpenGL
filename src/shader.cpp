#include "shader.hpp"

#include <fstream>
#include <sstream>

std::unordered_map<std::string, uint32_t> shader::precompiled_shaders;

bool shader::create(const char *vs_filepath, const char *fs_filepath) const noexcept {
    uint32_t vs_id = 0;
    
    if (precompiled_shaders.find(vs_filepath) != precompiled_shaders.cend()) {
        vs_id = precompiled_shaders.at(vs_filepath);
    } else {
        vs_id = _compile_shader(GL_VERTEX_SHADER, _read_shader_data_from_file(vs_filepath).c_str());
        if (vs_id == 0) {
            return false;
        }

        precompiled_shaders[vs_filepath] = vs_id;
    }

    uint32_t fs_id = 0;
    if (precompiled_shaders.find(fs_filepath) != precompiled_shaders.cend()) {
        fs_id = precompiled_shaders.at(fs_filepath);
    } else {
        fs_id = _compile_shader(GL_FRAGMENT_SHADER, _read_shader_data_from_file(fs_filepath).c_str());
        if (fs_id == 0) {
            return false;
        }

        precompiled_shaders[fs_filepath] = fs_id;
    }

    m_program_id = _create_shader_program(vs_id, fs_id);

    return m_program_id != 0;
}

bool shader::bind() const noexcept {
    const bool prev_state = m_is_binded;

    glUseProgram(m_program_id);
    m_is_binded = (m_program_id != 0);

    return prev_state;
}

bool shader::unbind() const noexcept {
    glUseProgram(0);
    return (m_is_binded = false);
}

bool shader::is_binded() const noexcept {
    return m_is_binded;
}

void shader::uniform(const char *name, bool value) const noexcept {
    _set_uniform(glUniform1i, name, value);
}

void shader::uniform(const char *name, float value) const noexcept {
    _set_uniform(glUniform1f, name, value);
}

void shader::uniform(const char *name, double value) const noexcept {
    _set_uniform(glUniform1d, name, value);
}

void shader::uniform(const char *name, uint32_t value) const noexcept {
    _set_uniform(glUniform1ui, name, value);
}

void shader::uniform(const char *name, const glm::vec3& value) const noexcept {
    _set_uniform(glUniform3fv, name, 1, &value.x);
}

void shader::uniform(const char *name, const glm::vec4& value) const noexcept {
    _set_uniform(glUniform4fv, name, 1, &value[0]);
}

void shader::uniform(const char *name, const glm::mat4& value) const noexcept {
    _set_uniform(glUniformMatrix4fv, name, 1, false, &value[0][0]);
}

std::string shader::_read_shader_data_from_file(const char *filepath) const noexcept {
    std::ifstream file(filepath);

    if (file.is_open()) {
        std::stringstream shader_stream;
        shader_stream << file.rdbuf();

        return shader_stream.str();
    }

    spdlog::error("invalid shader source filepath: {}", filepath);
    return "";
}

uint32_t shader::_compile_shader(GLenum shader_type, const char *source) const noexcept {
    const uint32_t id = glCreateShader(shader_type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    int32_t success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        int32_t error_msg_len = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &error_msg_len);

        char* error_msg = (char*)alloca(error_msg_len);
        glGetShaderInfoLog(id, error_msg_len, nullptr, error_msg);
        spdlog::error("{} shader compilation error: {}", (shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment"), error_msg);
        return 0;
    }

    return id;
}

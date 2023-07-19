#include "shader.hpp"

#include <fstream>
#include <sstream>

std::unordered_map<std::string, uint32_t> shader::precompiled_shaders;

void shader::create(const std::string& vs_filepath, const std::string& fs_filepath) const noexcept {
    uint32_t vs_id = _create_shader(GL_VERTEX_SHADER, vs_filepath);
    uint32_t fs_id = _create_shader(GL_FRAGMENT_SHADER, fs_filepath);

    const_cast<shader*>(this)->m_program_id = _create_shader_program(vs_id, fs_id);

#ifdef _DEBUG
    int32_t link_status;
    OGL_CALL(glGetProgramiv(m_program_id, GL_LINK_STATUS, &link_status));
    if (link_status == GL_FALSE) {
        int32_t log_len = 0;
        OGL_CALL(glGetProgramiv(m_program_id, GL_INFO_LOG_LENGTH, &log_len));

        char* shader_error_log = (char*)alloca(log_len);
        OGL_CALL(glGetProgramInfoLog(m_program_id, log_len, nullptr, shader_error_log));
        OGL_CALL(glDeleteProgram(m_program_id));
        
        ASSERT(false, "shader program linking error", shader_error_log);
    }
#endif
}

std::string shader::_read_shader_data_from_file(const std::string& filepath) noexcept {
    std::ifstream file(filepath);

    ASSERT(file.is_open(), "can't open file", filepath)

    std::stringstream shader_stream;
    shader_stream << file.rdbuf();

    return shader_stream.str();
}

uint32_t shader::_compile_shader(GLenum shader_type, const std::string& source) noexcept {
    uint32_t id;
    OGL_CALL(id = glCreateShader(shader_type));

    const GLchar* src = source.c_str();
    OGL_CALL(glShaderSource(id, 1, &src, nullptr));
    
    OGL_CALL(glCompileShader(id));
    
    return id;
}

uint32_t shader::_create_shader(GLenum shader_type, const std::string& filepath) noexcept {
    if (precompiled_shaders.find(filepath) != precompiled_shaders.cend()) {
        return precompiled_shaders.at(filepath);
    }
    
    const auto source = _read_shader_data_from_file(filepath);
    const uint32_t id = _compile_shader(shader_type, source.c_str());
    
#ifdef _DEBUG
    int32_t compilation_status;
    OGL_CALL(glGetShaderiv(id, GL_COMPILE_STATUS, &compilation_status));
        
    if (compilation_status == GL_FALSE) {
        int32_t log_len = 0;
        OGL_CALL(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_len));

        char* shader_error_log = (char*)alloca(log_len);
        OGL_CALL(glGetShaderInfoLog(id, log_len, nullptr, shader_error_log));
        OGL_CALL(glDeleteShader(id));

        ASSERT(false, std::string(filepath), shader_error_log);
    }
#endif

    precompiled_shaders[filepath] = id;
    return id;
}

uint32_t shader::get_id() const noexcept {
    return m_program_id;
}

void shader::bind() const noexcept {
    OGL_CALL(glUseProgram(m_program_id));
}

void shader::unbind() const noexcept {
    OGL_CALL(glUseProgram(0));
}

void shader::uniform(const std::string& name, bool uniform) const noexcept {
    _set_uniform(glUniform1ui, name, uniform);
}

void shader::uniform(const std::string& name, float uniform) const noexcept {
    _set_uniform(glUniform1f, name, uniform);
}

void shader::uniform(const std::string& name, double uniform) const noexcept {
    _set_uniform(glUniform1d, name, uniform);
}

void shader::uniform(const std::string& name, int32_t uniform) const noexcept {
    _set_uniform(glUniform1i, name, uniform);
}

void shader::uniform(const std::string& name, uint32_t uniform) const noexcept {
    _set_uniform(glUniform1ui, name, uniform);
}

void shader::uniform(const std::string& name, const glm::vec3& uniform) const noexcept {
    _set_uniform(glUniform3fv, name, 1, glm::value_ptr(uniform));
}

void shader::uniform(const std::string& name, const glm::vec4& uniform) const noexcept {
    _set_uniform(glUniform4fv, name, 1, glm::value_ptr(uniform));
}

void shader::uniform(const std::string& name, const glm::mat4& uniform) const noexcept {
    _set_uniform(glUniformMatrix4fv, name, 1, false, glm::value_ptr(uniform));
}

bool shader::operator==(const shader &shader) const noexcept {
    return m_program_id == shader.m_program_id;
}

bool shader::operator!=(const shader &shader) const noexcept {
    return !this->operator==(shader);
}
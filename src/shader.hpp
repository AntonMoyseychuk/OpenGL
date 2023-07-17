#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <spdlog/spdlog.h>

#include <cstdint>

#include <unordered_map>
#include <string>

class shader {
public:
    shader() = default;

    uint32_t create(const char* vs_filepath, const char* fs_filepath) const noexcept;
    uint32_t get_id() const noexcept;

    bool bind() const noexcept;
    bool unbind() const noexcept;
    bool is_binded() const noexcept;

    void uniform(const char* name, bool             uniform) const noexcept;
    void uniform(const char* name, float            uniform) const noexcept;
    void uniform(const char* name, double           uniform) const noexcept;
    void uniform(const char* name, int32_t          uniform) const noexcept;
    void uniform(const char* name, uint32_t         uniform) const noexcept;
    void uniform(const char* name, const glm::vec3& uniform) const noexcept;
    void uniform(const char* name, const glm::vec4& uniform) const noexcept;
    void uniform(const char* name, const glm::mat4& uniform) const noexcept;

    bool operator==(const shader& shader) const noexcept;
    bool operator!=(const shader& shader) const noexcept;

private:
    template <typename glUniformFunc, typename ... Args>
    void _set_uniform(glUniformFunc gl_uniform, const char* name, Args&&... args) const noexcept;

private:
    static std::string _read_shader_data_from_file(const char* filepath) noexcept;
    static uint32_t _compile_shader(GLenum shader_type, const char* source) noexcept;
    static uint32_t _create_shader(GLenum shader_type, const char* filepath) noexcept;

    template <typename ID, typename... IDs>
    static void _attach_shader(uint32_t program_id, ID first, IDs&&... args) noexcept;
    static void _attach_shader(uint32_t program_id) { }

    template <typename ... IDs>
    static uint32_t _create_shader_program(IDs&&... shader_ids) noexcept;

private:
    static std::unordered_map<std::string, uint32_t> precompiled_shaders;

private:
    mutable std::unordered_map<std::string, uint32_t> m_uniform_locations;
    mutable uint32_t m_program_id = 0;
    mutable bool m_is_binded = false;
};

template <typename glUniformFunc, typename... Args>
inline void shader::_set_uniform(glUniformFunc gl_uniform, const char *name, Args&&... args) const noexcept {
    const bool was_binded = bind();
    
    const std::string u_name(name);
    if (m_uniform_locations.find(u_name) != m_uniform_locations.cend()) {
        gl_uniform(m_uniform_locations.at(u_name), std::forward<Args>(args)...);
    } else {
        const int32_t unifrom_location = glGetUniformLocation(m_program_id, u_name.c_str());

        if (unifrom_location != -1) {
            m_uniform_locations[u_name] = unifrom_location;
            gl_uniform(unifrom_location, std::forward<Args>(args)...);
        }
    }

    if (!was_binded) {
       unbind();
    }
}

template <typename ID, typename... IDs>
inline void shader::_attach_shader(uint32_t program_id, ID first, IDs && ...args) noexcept {
    glAttachShader(program_id, first);
    _attach_shader(program_id, std::forward<IDs>(args)...);
}

template <typename... IDs>
inline uint32_t shader::_create_shader_program(IDs&&... shader_ids) noexcept {
    const size_t id = glCreateProgram();
    _attach_shader(id, std::forward<IDs>(shader_ids)...);
    glLinkProgram(id);

    int32_t success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        int32_t error_msg_len = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &error_msg_len);

        char* error_msg = (char*)alloca(error_msg_len);
        glGetProgramInfoLog(id, error_msg_len, nullptr, error_msg);
        spdlog::error("shader program linking error: {}", error_msg);
        return 0;
    }
    
    return id;
}
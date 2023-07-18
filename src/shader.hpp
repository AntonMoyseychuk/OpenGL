#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "debug.hpp"

#include <cstdint>

#include <unordered_map>
#include <string>
#include <optional>

class shader {
public:
    shader() = default;

    uint32_t create(const char* vs_filepath, const char* fs_filepath) const noexcept;
    uint32_t get_id() const noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

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
    static std::optional<std::string> _read_shader_data_from_file(const char* filepath) noexcept;
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
};

template <typename glUniformFunc, typename... Args>
inline void shader::_set_uniform(glUniformFunc gl_uniform, const char *name, Args&&... args) const noexcept {
    this->bind();
    
    const std::string u_name(name);
    if (m_uniform_locations.find(u_name) != m_uniform_locations.cend()) {
        OGL_CALL(gl_uniform(m_uniform_locations.at(u_name), std::forward<Args>(args)...));
    } else {
        int32_t uniform_location;
        OGL_CALL(uniform_location = glGetUniformLocation(m_program_id, u_name.c_str()));

        if (uniform_location != -1) {
            m_uniform_locations[u_name] = uniform_location;
            OGL_CALL(gl_uniform(uniform_location, std::forward<Args>(args)...));
        }
    }
}

template <typename ID, typename... IDs>
inline void shader::_attach_shader(uint32_t program_id, ID first, IDs && ...args) noexcept {
    OGL_CALL(glAttachShader(program_id, first));
    _attach_shader(program_id, std::forward<IDs>(args)...);
}

template <typename... IDs>
inline uint32_t shader::_create_shader_program(IDs&&... shader_ids) noexcept {
    uint32_t id;
    OGL_CALL(id = glCreateProgram());
    
    _attach_shader(id, std::forward<IDs>(shader_ids)...);
    
    OGL_CALL(glLinkProgram(id));

    return id;
}
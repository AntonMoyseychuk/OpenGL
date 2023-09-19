#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "debug.hpp"

#include <cstdint>

#include <unordered_map>
#include <string>
#include <optional>

#include "nocopyable.hpp"

class shader : public nocopyable {
public:
    shader() = default;
    shader(const std::string& vs_filepath, const std::string& fs_filepath, const std::optional<std::string>& gs_filepath = std::nullopt);

    ~shader();

    void create(const std::string& vs_filepath, const std::string& fs_filepath, const std::optional<std::string>& gs_filepath = std::nullopt) noexcept;
    void destroy() noexcept;
    uint32_t get_id() const noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

    void uniform(const std::string& name, bool             uniform) const noexcept;
    void uniform(const std::string& name, float            uniform) const noexcept;
    void uniform(const std::string& name, double           uniform) const noexcept;
    void uniform(const std::string& name, int32_t          uniform) const noexcept;
    void uniform(const std::string& name, uint32_t         uniform) const noexcept;
    void uniform(const std::string& name, const glm::vec2& uniform) const noexcept;
    void uniform(const std::string& name, const glm::vec3& uniform) const noexcept;
    void uniform(const std::string& name, const glm::vec4& uniform) const noexcept;
    void uniform(const std::string& name, const glm::mat3& uniform) const noexcept;
    void uniform(const std::string& name, const glm::mat4& uniform) const noexcept;

    shader(shader&& shader);
    shader& operator=(shader&& shader) noexcept;
    
    bool operator==(const shader& shader) const noexcept;
    bool operator!=(const shader& shader) const noexcept;

private:
    template <typename glUniformFunc, typename ... Args>
    void _set_uniform(glUniformFunc gl_uniform, const std::string& name, Args&&... args) const noexcept;

private:
    static std::string _read_shader_data_from_file(const std::string& filepath) noexcept;
    static uint32_t _compile_shader(GLenum shader_type, const std::string& source) noexcept;
    static uint32_t _create_shader(GLenum shader_type, const std::string& filepath) noexcept;

    template <typename ID, typename... IDs>
    static void _attach_shader(uint32_t program_id, ID first, IDs&&... args) noexcept;
    static void _attach_shader(uint32_t program_id) { }

    template <typename ... IDs>
    static uint32_t _create_shader_program(IDs&&... shader_ids) noexcept;

private:
    static std::unordered_map<std::string, uint32_t> precompiled_shaders;

private:
    std::unordered_map<std::string, uint32_t> m_uniform_locations;
    uint32_t m_program_id = 0;
};



template <typename glUniformFunc, typename... Args>
inline void shader::_set_uniform(glUniformFunc gl_uniform, const std::string& name, Args&&... args) const noexcept {
    this->bind();
    
    if (m_uniform_locations.find(name) != m_uniform_locations.cend()) {
        OGL_CALL(gl_uniform(m_uniform_locations.at(name), std::forward<Args>(args)...));
    } else {
        int32_t uniform_location;
        OGL_CALL(uniform_location = glGetUniformLocation(m_program_id, name.c_str()));

        if (uniform_location != -1) {
            const_cast<shader*>(this)->m_uniform_locations[name] = uniform_location;
            OGL_CALL(gl_uniform(uniform_location, std::forward<Args>(args)...));
        }
    }
}

template <typename ID, typename... IDs>
inline void shader::_attach_shader(uint32_t program_id, ID first, IDs && ...args) noexcept {
    if (first != 0) {
        OGL_CALL(glAttachShader(program_id, first));
    }
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
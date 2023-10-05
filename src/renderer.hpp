#pragma once
#include "shader.hpp"
#include "model.hpp"
#include "particle_system.hpp"

class renderer {
public:
    renderer() = default;
    
    void set_clear_color(float r, float g, float b, float a) const noexcept;
    void set_clear_color(const glm::vec4& color) const noexcept;
    void clear(uint32_t mask) const noexcept;
    
    void polygon_mode(uint32_t face, uint32_t mode) const noexcept;
    void blend_func(uint32_t sfactor, uint32_t dfactor) const noexcept;
    void depth_func(uint32_t func) const noexcept;
    void cull_face(uint32_t face) const noexcept;

    void enable(uint32_t flag) const noexcept;
    void disable(uint32_t flag) const noexcept;

    void viewport(int32_t x, int32_t y, uint32_t width, uint32_t height) const noexcept;

    void render(uint32_t mode, const shader& shader, const mesh& mesh) const noexcept;
    void render(uint32_t mode, const shader& shader, const model& model) const noexcept;
    void render(uint32_t mode, const shader& shader, const particle_system& particles) const noexcept;
    void render_instanced(uint32_t mode, const shader& shader, const mesh& mesh, size_t count) const noexcept;
    void render_instanced(uint32_t mode, const shader& shader, const model& model, size_t count) const noexcept;
};
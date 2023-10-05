#include "renderer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

void renderer::clear(uint32_t mask) const noexcept {
    OGL_CALL(glClear(mask));
}

void renderer::polygon_mode(uint32_t face, uint32_t mode) const noexcept {
    OGL_CALL(glPolygonMode(face, mode));
}

void renderer::blend_func(uint32_t sfactor, uint32_t dfactor) const noexcept {
    OGL_CALL(glBlendFunc(sfactor, dfactor));
}

void renderer::depth_func(uint32_t func) const noexcept {
    OGL_CALL(glDepthFunc(func));
}

void renderer::cull_face(uint32_t face) const noexcept {
    OGL_CALL(glCullFace(face));
}

void renderer::set_clear_color(float r, float g, float b, float a) const noexcept {
    OGL_CALL(glClearColor(r, g, b, a));
}

void renderer::set_clear_color(const glm::vec4 &color) const noexcept {
    set_clear_color(color.r, color.g, color.b, color.a);
}

void renderer::enable(uint32_t flag) const noexcept {
    OGL_CALL(glEnable(flag));
}

void renderer::disable(uint32_t flag) const noexcept {
    OGL_CALL(glDisable(flag));
}

void renderer::viewport(int32_t x, int32_t y, uint32_t width, uint32_t height) const noexcept {
    OGL_CALL(glViewport(x, y, width, height));
}

void renderer::render(uint32_t mode, const shader &shader, const mesh &mesh) const noexcept {
    mesh.bind(shader);

    const size_t index_count = mesh.ibo.get_element_count();
    if (index_count == 0) {
        const size_t vertex_count = mesh.vbo.get_element_count();
        if (vertex_count > 0) {
            OGL_CALL(glDrawArrays(mode, 0, vertex_count));
        }
    } else {
        OGL_CALL(glDrawElements(mode, index_count, GL_UNSIGNED_INT, nullptr));
    }
}

void renderer::render(uint32_t mode, const shader &shader, const model &model) const noexcept {
    const auto meshes = model.get_meshes();
    if (meshes == nullptr) {
        LOG_WARN("renderer", "meshes == nullptr");
        return;
    }
    
    for (size_t i = 0; i < meshes->size(); ++i) {
        render(mode, shader, meshes->at(i));
    }
}

void renderer::render(uint32_t mode, const shader &shader, const particle_system &particles) const noexcept {
    for (auto& particle : particles.m_particle_pool) {
        if (particle.is_active == false) {
            continue;
        }

        const float life = particle.life_remaining / particle.life_time;
        glm::vec4 color = glm::lerp(particle.end_color, particle.start_color, life);
        color.a *= life;

        const float size = glm::lerp(particle.end_size, particle.start_size, life);
        
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(particle.position.x, particle.position.y, 0.0f))
            * glm::rotate(glm::mat4(1.0f), particle.rotation, glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, size, 1.0f));
        shader.uniform("u_model", model);
        shader.uniform("u_color", color);
        this->render(mode, shader, particles.m_mesh);
    }
}

void renderer::render_instanced(uint32_t mode, const shader &shader, const mesh &mesh, size_t count) const noexcept {
    mesh.bind(shader);

    const size_t index_count = mesh.ibo.get_element_count();
    if (index_count == 0) {
        OGL_CALL(glDrawArraysInstanced(mode, 0, mesh.vbo.get_element_count(), count));
    } else {
        OGL_CALL(glDrawElementsInstanced(mode, index_count, GL_UNSIGNED_INT, nullptr, count));
    }
}

void renderer::render_instanced(uint32_t mode, const shader &shader, const model &model, size_t count) const noexcept {
    const auto meshes = model.get_meshes();
    if (meshes == nullptr) {
        LOG_WARN("renderer", "meshes == nullptr");
        return;
    }
    
    for (size_t i = 0; i < meshes->size(); ++i) {
        render_instanced(mode, shader, meshes->at(i), count);
    }
}

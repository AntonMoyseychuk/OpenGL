#include "renderer.hpp"

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

void renderer::set_clear_color(float r, float g, float b, float a) const noexcept {
    OGL_CALL(glClearColor(r, g, b, a));
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

    const uint32_t index_count = mesh.get_index_buffer().get_element_count();
    if (index_count == 0) {
        OGL_CALL(glDrawArrays(mode, 0, mesh.get_vertex_buffer().get_element_count()));
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

void renderer::render_instanced(uint32_t mode, const shader &shader, const mesh &mesh, size_t count) const noexcept {
    mesh.bind(shader);

    const uint32_t index_count = mesh.get_index_buffer().get_element_count();
    if (index_count == 0) {
        OGL_CALL(glDrawArraysInstanced(mode, 0, mesh.get_vertex_buffer().get_element_count(), count));
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

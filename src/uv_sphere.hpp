#pragma once
#include "mesh.hpp"

class uv_sphere {
public:
    uv_sphere() = default;
    uv_sphere(uint32_t stacks, uint32_t slices);

    void generate(uint32_t stacks, uint32_t slices) noexcept;
    void set_textures(const std::unordered_map<std::string, texture::config>& texture_configs) noexcept;

    const mesh& get_mesh() const noexcept;

private:
    mesh::vertex _create_vertex(float theta, float phi, float tex_s, float tex_t) const noexcept;

public:
    uint32_t stacks;
    uint32_t slices;

private:
    mesh m_mesh;
};
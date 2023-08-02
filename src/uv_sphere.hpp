#pragma once
#include "mesh.hpp"

class uv_sphere {
public:
    uv_sphere() = default;
    uv_sphere(uint32_t stacks, uint32_t slices);

    void generate(uint32_t stacks, uint32_t slices) noexcept;

    void draw(const shader& shader) const noexcept;

    const mesh& get_mesh() const noexcept;

public:
    uint32_t stacks;
    uint32_t slices;

private:
    mesh m_mesh;
};
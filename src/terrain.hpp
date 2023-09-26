#pragma once

#include "mesh.hpp"


struct terrain {
    terrain() = default;
    terrain(const std::string_view height_map_path, float world_scale, float height_scale);

    void create(const std::string_view height_map_path, float world_scale, float height_scale) noexcept;

    float get_height(float x, float z) const noexcept;
    float get_interpolated_height(float x, float z) const noexcept;

private:
    void _calculate_normals_from_verices_and_indices(std::vector<mesh::vertex>& vertices, const std::vector<std::uint32_t>& indices) noexcept;

public:
    mesh mesh;
    std::vector<float> heights;

    int32_t width = 0;
    int32_t depth = 0;

    float min_height = std::numeric_limits<float>::max();
    float max_height = std::numeric_limits<float>::min();

    float world_scale = 1.0f;
    float height_scale = 1.0f;
};
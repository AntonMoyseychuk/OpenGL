#pragma once

#include "mesh.hpp"


struct terrain {
    terrain() = default;
    terrain(const std::string_view height_map_path, float world_scale, float height_scale);

    void create(const std::string_view height_map_path, float world_scale, float height_scale) noexcept;
    void create_water_mesh(float height) noexcept;

    float get_height(float x, float z) const noexcept;
    float get_interpolated_height(float x, float z) const noexcept;

    void calculate_tile_regions(size_t tiles_count, const std::string* tile_texture_paths = nullptr) noexcept;

private:
    void _calculate_normals(std::vector<mesh::vertex>& vertices, const std::vector<std::uint32_t>& indices) noexcept;

public:
    struct tile {
        texture_2d texture;
        float low = 0.0f;
        float optimal = 0.0f;
        float high = 0.0f;
    };

    mesh ground_mesh;
    mesh water_mesh;
    std::vector<float> heights;
    std::vector<tile> tiles;

    int32_t width = 0;
    int32_t depth = 0;

    float min_height = std::numeric_limits<float>::max();
    float max_height = std::numeric_limits<float>::min();

    float world_scale = 1.0f;
    float height_scale = 1.0f;
};
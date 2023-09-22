#pragma once

#include "mesh.hpp"


struct terrain {
    terrain() = default;
    terrain(const std::string_view height_map_path, float world_scale, float height_scale);

    void create(const std::string_view height_map_path, float world_scale, float height_scale);

    mesh mesh;
    std::vector<float> heights;

    int32_t width = 0;
    int32_t depth = 0;

    float world_scale = 1.0f;
    float height_scale = 1.0f;
};
#include "terrain.hpp"

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

terrain::terrain(const std::string_view height_map_path, float world_scale, float height_scale) {
    create(height_map_path, world_scale, height_scale);
}

void terrain::create(const std::string_view height_map_path, float world_scale, float height_scale) noexcept {
    int32_t channel_count;
    uint8_t* height_map = stbi_load(height_map_path.data(), &width, &depth, &channel_count, 0);

    ASSERT(height_map != nullptr, "terrain", stbi_failure_reason());

    if (width == 0 || depth == 0) {
        stbi_image_free(height_map);
        return;
    }

    ASSERT(world_scale >= 0.0f, "terrain", "world_scale must be greater or equal than zero");
    ASSERT(height_scale >= 0.0f, "terrain", "height_scale must be greater or equal than zero");
    this->world_scale = world_scale;
    this->height_scale = height_scale;

    const float du = 1.0f / (world_scale), dv = 1.0f / (world_scale);

    std::vector<mesh::vertex> vertices(width * depth);
    this->heights.resize(vertices.size());
    for (uint32_t z = 0; z < depth; ++z) {
        for (uint32_t x = 0; x < width; ++x) {
            const size_t index = z * width * channel_count + x * channel_count;
            const float height = (height_map[index + 1]) * height_scale;

            min_height = std::min(min_height, height);
            max_height = std::max(max_height, height);

            this->heights[z * width + x] = height;

            mesh::vertex& vertex = vertices[z * width + x];
            vertex.position = glm::vec3(x * world_scale, height, z * world_scale);
            vertex.texcoord = glm::vec2(x * du, (depth - 1 - z) * dv);
        }
    }
    stbi_image_free(height_map);

    std::vector<uint32_t> indices;
    indices.reserve((width - 1) * (depth - 1) * 6);
    for (uint32_t z = 0; z < depth - 1; ++z) {
        for (uint32_t x = 0; x < width - 1; ++x) {
            indices.emplace_back(z * width + x);
            indices.emplace_back((z + 1) * width + x);
            indices.emplace_back(z * width + (x + 1));

            indices.emplace_back(z * width + (x + 1));
            indices.emplace_back((z + 1) * width + x);
            indices.emplace_back((z + 1) * width + (x + 1));
        }
    }

    _calculate_normals(vertices, indices);

    ground_mesh.create(vertices, indices);
}

void terrain::create_water_mesh(float height) noexcept {
    std::vector<mesh::vertex> vertices(width * depth);
    
    for (uint32_t z = 0; z < depth; ++z) {
        for (uint32_t x = 0; x < width; ++x) {
            mesh::vertex& vertex = vertices[z * width + x];
            vertex.position = glm::vec3(x, height, z);
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vertex.texcoord = glm::vec2(x / static_cast<float>(width - 1), (depth - 1 - z) / static_cast<float>(depth - 1));
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve((width - 1) * (depth - 1) * 6);
    for (uint32_t z = 0; z < depth - 1; ++z) {
        for (uint32_t x = 0; x < width - 1; ++x) {
            indices.emplace_back(z * width + x);
            indices.emplace_back((z + 1) * width + x);
            indices.emplace_back(z * width + (x + 1));

            indices.emplace_back(z * width + (x + 1));
            indices.emplace_back((z + 1) * width + x);
            indices.emplace_back((z + 1) * width + (x + 1));
        }
    }

    water_mesh.create(vertices, indices);
}

float terrain::get_height(float x, float z) const noexcept {
    x = glm::clamp(x, 0.0f, width * world_scale);
    z = glm::clamp(z, 0.0f, (depth - 1) * world_scale);
    return heights[size_t(z / world_scale) * width + size_t(x / world_scale)];
}

float terrain::get_interpolated_height(float x, float z) const noexcept {
    const float base_height = get_height(x, z);
    if (x + 1.0f >= width * world_scale || z + 1.0f >= depth * world_scale) {
        return base_height;
    }

    const float next_height_x = get_height(x + 1.0f, z);
    const float ratio_x = x - floorf(x);
    const float interpolated_height_x = (next_height_x - base_height) * ratio_x + base_height;

    const float next_height_z = get_height(x, z + 1.0f);
    const float ratio_z = z - floorf(z);
    const float interpolated_height_z = (next_height_z - base_height) * ratio_z + base_height;

    const float final_height = (interpolated_height_x + interpolated_height_z) / 2.0f;

    return final_height;
}

void terrain::calculate_tile_regions(size_t tiles_count, const std::string* tile_texture_paths) noexcept {
    tiles.resize(tiles_count);

    const float height_range = max_height - min_height;

    const float tile_range = height_range /tiles_count;
    const float remainder = height_range - tile_range * tiles_count;

    ASSERT(remainder >= 0.0, "terrain", "negative remainder");

    float last_height = min_height - 1.0f;
    for (size_t i = 0; i < tiles_count; ++i) {
        tiles[i].low = last_height + 1;
        last_height += tile_range;
        tiles[i].optimal = last_height;
        tiles[i].high = tiles[i].optimal + tile_range;
    }

    if (tile_texture_paths != nullptr) {
        for (size_t i = 0; i < tiles_count; ++i) {
            tiles[i].texture.load(tile_texture_paths[i], true, false, texture_2d::variety::NONE);
            tiles[i].texture.generate_mipmap();
            tiles[i].texture.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            tiles[i].texture.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            tiles[i].texture.set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
            tiles[i].texture.set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
    }
}

void terrain::_calculate_normals(std::vector<mesh::vertex> &vertices, const std::vector<std::uint32_t> &indices) noexcept {
    struct vertex_normals_sum {
        glm::vec3 sum = glm::vec3(0.0f);
        uint32_t count = 0;
    };

    std::vector<vertex_normals_sum> averaged_normals(vertices.size());

    for (size_t i = 0; i < indices.size(); i += 3) {
        const glm::vec3 e0 = glm::normalize(vertices[indices[i + 0]].position - vertices[indices[i + 1]].position);
        const glm::vec3 e1 = glm::normalize(vertices[indices[i + 2]].position - vertices[indices[i + 1]].position);
            
        const glm::vec3 normal = glm::cross(e1, e0);

        averaged_normals[indices[i + 0]].sum += normal;
        averaged_normals[indices[i + 1]].sum += normal;
        averaged_normals[indices[i + 2]].sum += normal;
        ++averaged_normals[indices[i + 0]].count;
        ++averaged_normals[indices[i + 1]].count;
        ++averaged_normals[indices[i + 2]].count;
    }

    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].normal = glm::normalize(averaged_normals[i].sum / static_cast<float>(averaged_normals[i].count));
    }
}
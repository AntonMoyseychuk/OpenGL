#include "terrain.hpp"

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

terrain::terrain(const std::string_view height_map_path, float dudv) {
    create(height_map_path, dudv);
}

void terrain::create(const std::string_view height_map_path, float dudv) noexcept {
    int32_t channel_count;
    uint8_t* height_map = stbi_load(height_map_path.data(), &width, &depth, &channel_count, 0);

    ASSERT(height_map != nullptr, "terrain", stbi_failure_reason());

    if (width == 0 || depth == 0) {
        stbi_image_free(height_map);
        return;
    }

    ASSERT(dudv > 0.0f, "terrain", "dudv must be greater than zero");

    std::vector<mesh::vertex> vertices(width * depth);
    this->heights.resize(vertices.size());
    for (uint32_t z = 0; z < depth; ++z) {
        for (uint32_t x = 0; x < width; ++x) {
            const size_t index = z * width * channel_count + x * channel_count;
            const float height = height_map[index + 1];

            min_height = std::min(min_height, height);
            max_height = std::max(max_height, height);

            this->heights[z * width + x] = height;

            mesh::vertex& vertex = vertices[z * width + x];
            vertex.position = glm::vec3(x, height, z);
            vertex.texcoord = glm::vec2(x * dudv, (depth - 1 - z) * dudv);
        }
    }
    stbi_image_free(height_map);

    const auto indices = _generate_mesh_indices(width, depth);
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

    water_mesh.create(vertices, _generate_mesh_indices(width, depth));
}

float terrain::get_height(float local_x, float local_z) const noexcept {
    if (!_belongs_terrain(local_x, local_z)) {
        return std::numeric_limits<float>::lowest();
    }
    return heights[size_t(local_z) * width + size_t(local_x)];
}

float terrain::get_interpolated_height(float local_x, float local_z) const noexcept {
    const float base_height = get_height(local_x, local_z);
    if (!_belongs_terrain(local_x, local_z)) {
        return base_height;
    }

    const float next_height_x = get_height(local_x + 1.0f, local_z);
    const float ratio_x = local_x - floorf(local_x);
    const float interpolated_height_x = (next_height_x - base_height) * ratio_x + base_height;

    const float next_height_z = get_height(local_x, local_z + 1.0f);
    const float ratio_z = local_z - floorf(local_z);
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

std::vector<uint32_t> terrain::_generate_mesh_indices(int32_t width, int32_t depth) const noexcept {
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

    return indices;
}

void terrain::_calculate_normals(std::vector<mesh::vertex> &vertices, const std::vector<std::uint32_t> &indices) noexcept
{
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

bool terrain::_belongs_terrain(float local_x, float local_z) const noexcept {
    return (local_x >= 0.0f || local_x < width || local_z >= 0.0f || local_z < depth);
}

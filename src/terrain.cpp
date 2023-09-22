#include "terrain.hpp"

#include <stb/stb_image.h>

terrain::terrain(const std::string_view height_map_path, float world_scale, float height_scale) {
    create(height_map_path, world_scale, height_scale);
}

void terrain::create(const std::string_view height_map_path, float world_scale, float height_scale) {
    int32_t channel_count;
    uint8_t* height_map = stbi_load(height_map_path.data(), &width, &depth, &channel_count, 0);

    if (width == 0 || depth == 0) {
        stbi_image_free(height_map);
        return;
    }

    ASSERT(world_scale >= 0.0f, "terrain", "world_scale must be greater or equal than zero");
    ASSERT(height_scale >= 0.0f, "terrain", "height_scale must be greater or equal than zero");
    this->world_scale = world_scale;
    this->height_scale = height_scale;

    std::vector<mesh::vertex> vertices(width * depth);
    this->heights.resize(vertices.size());
    for (uint32_t z = 0; z < depth; ++z) {
        for (uint32_t x = 0; x < width; ++x) {
            const size_t index = z * width * channel_count + x * channel_count;
            const float height = (height_map[index]) * height_scale;

            this->heights[z * width + x] = height;

            mesh::vertex& vertex = vertices[z * width + x];
            vertex.position = glm::vec3(x * world_scale, height, z * world_scale);
            vertex.texcoord = glm::vec2(x, depth - 1 - z);
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
        vertices[i].normal = averaged_normals[i].sum / static_cast<float>(averaged_normals[i].count);
    }

    stbi_image_free(height_map);

    this->mesh.create(vertices, indices);
}

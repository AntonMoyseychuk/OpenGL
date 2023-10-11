#pragma once
#include <vector>

#include "mesh.hpp"
#include "camera.hpp"

struct particle_props {
    glm::vec3 position;
    glm::vec3 velocity, velocity_variation;
    glm::vec4 start_color, end_color;
    float start_size, end_size, size_variation;
    float life_time = 1.0f;
};

class particle_system {
    friend class renderer;
public:
    particle_system(size_t particle_count);

    void update(float dt, const camera& camera) noexcept;
    void emit(const particle_props& props) noexcept;

    void bind_buffers() const noexcept; 

    void set_atlas_tiles_count(uint32_t count) noexcept;

private:
    struct particle {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 start_color, end_color;
        float start_size, end_size;
        float rotation = 0.0f;
        float life_time = 1.0f;
        float life_remaining = 0.0f;

        bool is_active = false;
    };

    std::vector<particle> m_particle_pool;

    mesh m_mesh;
    buffer m_colors_buffer;
    buffer m_transforms_buffer;
    buffer m_offsets_buffer;
    buffer m_blending_buffer;

    uint32_t m_atlas_tiles_count = 1;

    size_t m_pool_index;
    size_t active_particles_count = 0;
};
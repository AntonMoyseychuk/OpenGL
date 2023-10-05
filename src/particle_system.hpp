#pragma once
#include <vector>

#include "mesh.hpp"

struct particle_props {
    glm::vec2 position;
    glm::vec2 velocity, velocity_variation;
    glm::vec4 start_color, end_color;
    float start_size, end_size, size_variation;
    float life_time = 1.0f;
};

class particle_system {
    friend class renderer;
public:
    particle_system(size_t particle_count);

    void update(float dt) noexcept;

    void emit(const particle_props& props) noexcept;

private:
    struct particle {
        glm::vec2 position;
        glm::vec2 velocity;
        glm::vec4 start_color, end_color;
        float start_size, end_size;
        float rotation = 0.0f;
        float life_time = 1.0f;
        float life_remaining = 0.0f;

        bool is_active = false;
    };

    std::vector<particle> m_particle_pool;
    size_t m_pool_index;

    mesh m_mesh;
};
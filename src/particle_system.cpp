#include "particle_system.hpp"

#include "random.hpp"

particle_system::particle_system(size_t particle_count) {
    ASSERT(particle_count > 0, "particle_system", "particle_count is equal 0");

    std::vector<mesh::vertex> vertices = {
        mesh::vertex{glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
        mesh::vertex{glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
        mesh::vertex{glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
        mesh::vertex{glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        0, 2, 3
    };

    m_mesh.create(vertices, indices);

    m_pool_index = particle_count - 1;
    m_particle_pool.resize(particle_count);
}

void particle_system::update(float dt) noexcept {
    for (auto& particle : m_particle_pool) {
        if (particle.is_active == false) {
            continue;
        }

        if (particle.life_remaining <= 0.0f) {
            particle.is_active = false;
            continue;
        }

        particle.life_remaining -= dt;
        particle.position += particle.velocity * dt;
        particle.rotation += 0.01f * dt;
    }
}

void particle_system::emit(const particle_props &props) noexcept {
    particle& particle = m_particle_pool[m_pool_index];
    
    particle.is_active = true;
    particle.position = props.position;
    particle.rotation = random(0.0f, 1.0f) * 2.0f * glm::pi<float>();

    particle.velocity = props.velocity;
    particle.velocity.x += props.velocity_variation.x * (random(0.0f, 1.0f) - 0.5f);
    particle.velocity.y += props.velocity_variation.y * (random(0.0f, 1.0f) - 0.5f);

    particle.start_color = props.start_color;
    particle.end_color = props.end_color;

    particle.life_time = props.life_time;
    particle.life_remaining = props.life_time;
    particle.start_size = props.start_size + props.size_variation * (random(0.0f, 1.0f) - 0.5f);
    particle.end_size = props.end_size;

    m_pool_index = --m_pool_index % m_particle_pool.size();
}

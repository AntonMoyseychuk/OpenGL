#include "particle_system.hpp"

#include "random.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

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
    m_colors_pool.resize(particle_count);
    m_transforms_pool.resize(particle_count);

    m_colors_buffer.create(GL_SHADER_STORAGE_BUFFER, particle_count * sizeof(m_colors_pool[0]), sizeof(m_colors_pool[0]), GL_STREAM_DRAW, nullptr);
    m_transforms_buffer.create(GL_SHADER_STORAGE_BUFFER, particle_count * sizeof(m_transforms_pool[0]), sizeof(m_transforms_pool[0]), GL_DYNAMIC_DRAW, nullptr);
}

void particle_system::update(float dt) noexcept {
    active_particles_count = 0;
    for (size_t i = 0; i < m_particle_pool.size(); ++i) {
        particle_system::particle& particle = m_particle_pool[i];

        if (particle.is_active == false) {
            continue;
        }

        if (particle.life_remaining <= 0.0f) {
            particle.is_active = false;
            continue;
        }

        ++active_particles_count;

        size_t insertion_index = active_particles_count - 1;

        particle.life_remaining -= dt;
        particle.position += particle.velocity * dt;
        particle.rotation += 0.01f * dt;

        const float life = particle.life_remaining / particle.life_time;
        m_colors_pool[insertion_index] = glm::lerp(particle.end_color, particle.start_color, life);
        m_colors_pool[insertion_index].a *= life;

        const float size = glm::lerp(particle.end_size, particle.start_size, life);
        
        m_transforms_pool[insertion_index] = glm::translate(glm::mat4(1.0f), glm::vec3(particle.position.x, particle.position.y, 0.0f))
            * glm::rotate(glm::mat4(1.0f), particle.rotation, glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, size, 1.0f));
    }

    if (active_particles_count > 0) {
        m_colors_buffer.subdata(0, active_particles_count * sizeof(m_colors_pool[0]), m_colors_pool.data());
        m_transforms_buffer.subdata(0, active_particles_count * sizeof(m_transforms_pool[0]), m_transforms_pool.data());
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

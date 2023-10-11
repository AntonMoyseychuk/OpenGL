#include "particle_system.hpp"

#include "random.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

#include <map>

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

    m_colors_buffer.create(GL_SHADER_STORAGE_BUFFER, particle_count * sizeof(glm::vec4), sizeof(glm::vec4), GL_STREAM_DRAW, nullptr);
    m_transforms_buffer.create(GL_SHADER_STORAGE_BUFFER, particle_count * sizeof(glm::mat4), sizeof(glm::mat4), GL_DYNAMIC_DRAW, nullptr);
}

namespace util {
    template <typename V, typename Iterator>
    static void copy_map_into_buffer(const Iterator& begin, const Iterator& end, V* buffer) {
        ASSERT(buffer != nullptr, "copy_map_into_buffer", "buffer == nullptr");
        
        auto it = begin;
        for (size_t i = 0; it != end; ++it, ++i) {
            buffer[i] = it->second;
        }
    }
}

void particle_system::update(float dt, const camera& camera) noexcept {
    std::map<double, glm::mat4> sorted_transforms;
    std::map<double, glm::vec4> sorted_colors;
    
    active_particles_count = 0;

    const glm::dvec3 high_precision_camera_position = camera.position;
    
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

        particle.life_remaining -= dt;
        particle.position += particle.velocity * dt;
        particle.rotation += 0.01f * dt;

        const double particle_to_camera_distance = glm::distance(high_precision_camera_position, glm::dvec3(particle.position));

        const float life = particle.life_remaining / particle.life_time;
        glm::vec4 color = glm::lerp(particle.end_color, particle.start_color, life);
        color.a *= life;
        
        sorted_colors.insert_or_assign(particle_to_camera_distance, color);
        
        glm::mat4 transform(1.0f);
        const glm::mat4 view = camera.get_view();
        for (size_t y = 0; y < 3; ++y) {
            for (size_t x = 0; x < 3; ++x) {
                transform[y][x] = view[x][y];
            }
        }
        const float size = glm::lerp(particle.end_size, particle.start_size, life);
        transform *= glm::translate(glm::mat4(1.0f), particle.position) 
            * glm::rotate(glm::mat4(1.0f), particle.rotation, glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, size, 1.0f));

        sorted_transforms.insert_or_assign(particle_to_camera_distance, transform);
    }

    if (active_particles_count > 0) {
        util::copy_map_into_buffer(sorted_colors.rbegin(), sorted_colors.rend(), (glm::vec4*)m_colors_buffer.map(GL_WRITE_ONLY));
        util::copy_map_into_buffer(sorted_transforms.rbegin(), sorted_transforms.rend(), (glm::mat4*)m_transforms_buffer.map(GL_WRITE_ONLY));

        assert(m_colors_buffer.unmap());
        assert(m_transforms_buffer.unmap());
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
    particle.velocity.z += props.velocity_variation.z * (random(0.0f, 1.0f) - 0.5f);

    particle.start_color = props.start_color;
    particle.end_color = props.end_color;

    particle.life_time = props.life_time;
    particle.life_remaining = props.life_time;
    particle.start_size = props.start_size + props.size_variation * (random(0.0f, 1.0f) - 0.5f);
    particle.end_size = props.end_size;

    m_pool_index = --m_pool_index % m_particle_pool.size();
}

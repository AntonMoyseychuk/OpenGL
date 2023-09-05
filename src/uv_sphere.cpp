#include "uv_sphere.hpp"
#include <glm/gtc/constants.hpp>

uv_sphere::uv_sphere(uint32_t stacks, uint32_t slices) {
    generate(stacks, slices);
}

void uv_sphere::generate(uint32_t stacks, uint32_t slices) noexcept {
    this->stacks = stacks;
    this->slices = slices;

    const float theta_step = glm::pi<float>() / stacks;
    const float phi_step = glm::two_pi<float>() / slices;

    const float texcoord_s_step = 1.0f / slices;
    const float texcoord_t_step = 1.0f / stacks;

    std::vector<mesh::vertex> vertices;
    vertices.reserve((stacks - 1) * slices + 2);

    vertices.emplace_back(mesh::vertex { glm::vec3(0.0f, 1.0f, 0.0f), {}, { 0.5f, 1.0f } });
    for (uint32_t stack = 0; stack < stacks - 1; ++stack) {
        const float theta = (stack + 1) * theta_step;
        const float tex_t = (1.0f - texcoord_t_step) - stack * texcoord_t_step;

        for (uint32_t slice = 0; slice < slices; ++slice) {
            vertices.emplace_back(_create_vertex(theta, slice * phi_step, slice * texcoord_s_step, tex_t));
        }
    }
    vertices.emplace_back(mesh::vertex { glm::vec3(0.0f, -1.0f, 0.0f), {}, { 0.5f, 0.0f } });

    std::vector<uint32_t> indices;
    indices.reserve((2 * slices * 3) + ((stacks - 2) * slices * 6));

    for (uint32_t i = 0; i < slices; ++i) {
        const uint32_t a = i + 1;
        const uint32_t b = (i + 1) % slices + 1;
        
        indices.emplace_back(0);
        indices.emplace_back(a);
        indices.emplace_back(b);
    }

    for(uint32_t i = 0; i < stacks - 2; ++i) {	
		uint32_t a_start = i * slices;
		uint32_t b_start = (i + 1) * slices;
		
        for(uint32_t j = 0; j < slices; ++j) {
			uint32_t a0 = a_start + j + 1;
			uint32_t a1 = a_start + (j + 1) % slices + 1;
			uint32_t b0 = b_start + j + 1;
			uint32_t b1 = b_start + (j + 1) % slices + 1;

            indices.emplace_back(a0);
            indices.emplace_back(b1);
            indices.emplace_back(a1);

            indices.emplace_back(a0);
            indices.emplace_back(b0);
            indices.emplace_back(b1);
		}
	}

    const uint32_t bottom_index = vertices.size() - 1;
    for (uint32_t i = 0; i < slices; ++i) {
        const uint32_t a = i + bottom_index - slices;
        const uint32_t b = (i + 1) % slices + bottom_index - slices;
        
        indices.emplace_back(bottom_index);
        indices.emplace_back(b);
        indices.emplace_back(a);
    }

    ASSERT(indices.size() >= 3, "uv sphere generation error", "index count < 3");
    ASSERT(indices.size() % 3 == 0, "uv sphere generation error", "index count % 3 != 0");

    for (size_t i = 2; i < indices.size(); i += 3) {
        const glm::vec3 a = vertices[indices[i - 1]].position - vertices[indices[i - 2]].position;
        const glm::vec3 b = vertices[indices[i - 0]].position - vertices[indices[i - 2]].position;
        const glm::vec3 normal = glm::normalize(glm::cross(a, b));

        vertices[indices[i - 0]].normal = vertices[indices[i - 1]].normal = vertices[indices[i - 2]].normal = normal;
    }

    m_mesh.create(vertices, indices);
}

void uv_sphere::add_texture(texture&& tex) noexcept {
    m_mesh.add_texture(std::forward<texture>(tex));
}

const mesh &uv_sphere::get_mesh() const noexcept {
    return m_mesh;
}

mesh::vertex uv_sphere::_create_vertex(float theta, float phi, float tex_s, float tex_t) const noexcept {
    mesh::vertex v;
    v.position.x = glm::sin(theta) * glm::sin(phi);
    v.position.y = glm::cos(theta);
    v.position.z = glm::sin(theta) * glm::cos(phi);

    v.texcoord.x = tex_s;
    v.texcoord.y = tex_t;

    return v;
}

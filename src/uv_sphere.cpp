#include "uv_sphere.hpp"
#include <glm/gtc/constants.hpp>

uv_sphere::uv_sphere(uint32_t stacks, uint32_t slices) {
    this->generate(stacks, slices);
}

void uv_sphere::generate(uint32_t stacks, uint32_t slices) noexcept {
    this->stacks = stacks;
    this->slices = slices;

    const float pi_div_stacks = glm::pi<float>() / this->stacks;
    const float two_pi_div_slices = glm::two_pi<float>() / this->slices;

    std::vector<mesh::vertex> vertices;
    vertices.reserve(this->stacks * this->slices);

    std::vector<uint32_t> indices;

    vertices.emplace_back(mesh::vertex { glm::vec3(0.0f, 1.0f, 0.0f), {}, {} });
    for (uint32_t stack = 0; stack < this->stacks - 1; ++stack) {
        const float theta = stack * pi_div_stacks;

        for (uint32_t slice = 0; slice < this->slices; ++slice) {
            const float phi = slice * two_pi_div_slices;

            mesh::vertex v;
            v.position.x = glm::sin(theta) * glm::sin(phi);
            v.position.y = glm::cos(theta);
            v.position.z = glm::sin(theta) * glm::cos(phi);

            vertices.emplace_back(v);
        }
    }
    vertices.emplace_back(mesh::vertex { glm::vec3(0.0f, -1.0f, 0.0f), {}, {} });

    for (uint32_t i = 0; i < this->slices; ++i) {
        const uint32_t a = i + 1;
        const uint32_t b = (i + 1) % this->slices + 1;
        
        indices.push_back(0);
        indices.push_back(b);
        indices.push_back(a);
    }

    for(uint32_t i = 0; i < this->stacks - 2; ++i) {	
		uint32_t a_start = i * this->slices + 1;
		uint32_t b_start = (i + 1) * this->slices + 1;
		for(uint32_t j = 0; j < this->slices; j++) {
			uint32_t a0 = a_start + j;
			uint32_t a1 = a_start + (j + 1) % this->slices;
			uint32_t b0 = b_start + j;
			uint32_t b1 = b_start + (j + 1) % this->slices;

            indices.push_back(a0);
            indices.push_back(a1);
            indices.push_back(b1);

            indices.push_back(a0);
            indices.push_back(b1);
            indices.push_back(b0);
		}
	}

    for (uint32_t i = 0; i < this->slices; ++i) {
        const uint32_t a = i + (this->stacks - 2) * this->slices + 1;
        const uint32_t b = (i + 1) % this->slices + (this->stacks - 2) * this->slices + 1;
        
        indices.push_back(vertices.size() - 1);
        indices.push_back(a);
        indices.push_back(b);
    }

    ASSERT(indices.size() >= 3, "uv sphere generation error", "index count < 3");

    for (size_t i = 0; i < indices.size(); i += 3) {
        const glm::vec3 a = vertices[indices[i + 2]].position - vertices[indices[i + 0]].position;
        const glm::vec3 b = vertices[indices[i + 1]].position - vertices[indices[i + 0]].position;
        const glm::vec3 normal = glm::normalize(glm::cross(a, b));

        vertices[indices[i + 0]].normal = vertices[indices[i + 1]].normal = vertices[indices[i + 2]].normal = normal;
    }

    m_mesh.create(vertices, indices, {});
}

void uv_sphere::draw(const shader &shader) const noexcept {
    m_mesh.draw(shader);
}

const mesh &uv_sphere::get_mesh() const noexcept {
    return m_mesh;
}

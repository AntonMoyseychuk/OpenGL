#include "csm.hpp"

#include "debug.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

csm::csm(size_t cascade_count, const shadowmap_config& config) {
    create(cascade_count, config);
}

void csm::create(size_t cascade_count, const shadowmap_config& config) noexcept {
    ASSERT(cascade_count > 0, "csm", "cascade count must be greater than 0");
    
    data.width = config.width;
    data.height = config.height;

    fbo.create();

    shadowmaps.resize(cascade_count);
    subfrustas.resize(cascade_count);
    for (size_t i = 0; i < cascade_count; ++i) {
        texture_2d shadowmap(data.width, data.height, 0, config.internal_format, config.format, config.type);
        shadowmap.set_parameter(GL_TEXTURE_WRAP_S, config.wrap_s);
        shadowmap.set_parameter(GL_TEXTURE_WRAP_T, config.wrap_t);
        shadowmap.set_parameter(GL_TEXTURE_MAG_FILTER, config.mag_filter);
        shadowmap.set_parameter(GL_TEXTURE_MIN_FILTER, config.min_filter);
        shadowmap.set_parameter(GL_TEXTURE_BORDER_COLOR, &config.border_color.r);
        shadowmaps[i] = std::move(shadowmap);
    }

    fbo.attach(GL_DEPTH_ATTACHMENT, 0, shadowmaps[0]);

    fbo.set_draw_buffer(GL_NONE);
    fbo.set_read_buffer(GL_NONE);
}

void csm::calculate_subfrustas(
    float fov, float aspect, float near, float far, const glm::mat4& view, const glm::vec3& light_direction, float z_mult
) noexcept {
    ASSERT(fov > 0, "csm", "fov less or equal zero");
    ASSERT(aspect > 0, "csm", "aspect less or equal zero");
    for (size_t i = 0; i < subfrustas.size(); ++i) {
        const float n = i == 0 ? near : far / subfrustas.size() * i;
        const float f = far / subfrustas.size() * (i + 1);

        const glm::mat4 subfrusta_projection = glm::perspective(fov, aspect, n, f);

        subfrustas[i] = _get_subfrusta_matrices(view, subfrusta_projection, light_direction, z_mult);
        subfrustas[i].far = f;
    }
}

void csm::bind_for_writing(size_t cascade_index) const noexcept {
    ASSERT(cascade_index < shadowmaps.size(), "csm", "invalid cascade index");
    fbo.attach(GL_DEPTH_ATTACHMENT, 0, shadowmaps[cascade_index]); 
}

void csm::bind_for_reading(size_t first_slot) const noexcept {
    for (size_t i = 0; i < shadowmaps.size(); ++i) {
        shadowmaps[i].bind(int32_t(first_slot + i));
    }
}

std::vector<glm::vec4> csm::_get_subfrusta_corners_world_space(const glm::mat4& view, const glm::mat4& projection) const noexcept {
    std::vector<glm::vec4> corners;
    corners.reserve(8);
    
    const glm::mat4 inv = glm::inverse(projection * view);

    for (uint32_t x = 0; x < 2; ++x) {
        for (uint32_t y = 0; y < 2; ++y) {
            for (uint32_t z = 0; z < 2; ++z) {
                const glm::vec4 corner = inv * glm::vec4(
                    2.0f * x - 1.0f, 
                    2.0f * y - 1.0f, 
                    2.0f * z - 1.0f, 
                    1.0f);
                corners.emplace_back(corner / corner.w);
            }
        }
    }
    
    return std::move(corners);
}

csm::subfrusta csm::_get_subfrusta_matrices(
    const glm::mat4& subfrusta_view, const glm::mat4& subfrusta_projection, const glm::vec3& light_direction, float z_mult
) const noexcept {
    const auto frustum_corners = _get_subfrusta_corners_world_space(subfrusta_view, subfrusta_projection);
    glm::vec3 center(0.0f);
    for (const auto& corner : frustum_corners) {
        center += glm::vec3(corner);
    }
    center /= frustum_corners.size();

    const glm::mat4 light_view = glm::lookAt(center + glm::normalize(-light_direction), center, glm::vec3(0.0f, 1.0f, 0.0f));

    float min_x = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float min_y = std::numeric_limits<float>::max();
    float max_y = std::numeric_limits<float>::lowest();
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();
    for (const auto& corner : frustum_corners) {
        const auto trf = light_view * corner;
        min_x = std::min(min_x, trf.x);
        max_x = std::max(max_x, trf.x);
        min_y = std::min(min_y, trf.y);
        max_y = std::max(max_y, trf.y);
        min_z = std::min(min_z, trf.z);
        max_z = std::max(max_z, trf.z);
    }
        
    min_z *= min_z < 0 ? z_mult : (1.0f / z_mult);
    max_z *= max_z < 0 ? (1.0f / z_mult) : z_mult;

    const glm::mat4 light_projection = glm::ortho(min_x, max_x, min_y, max_y, min_z, max_z);

    return subfrusta{ light_view, light_projection };
}

csm::csm(csm &&csm) noexcept
    : fbo(std::move(csm.fbo)), shadowmaps(std::move(csm.shadowmaps)), data(csm.data)
{
    if (this != &csm) {
        memset(&csm.data, 0, sizeof(csm.data));
    }
}

csm &csm::operator=(csm &&csm) noexcept {
    if (this != &csm) {
        fbo = std::move(csm.fbo);
        shadowmaps = std::move(csm.shadowmaps);
        data = csm.data;

        memset(&csm.data, 0, sizeof(csm.data));
    }

    return *this;
}
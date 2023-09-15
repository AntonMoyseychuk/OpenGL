#pragma once

#include "texture.hpp"
#include "framebuffer.hpp"

#include <glm/glm.hpp>

struct csm {
    struct shadowmap_config {
        uint32_t width;
        uint32_t height;
        int32_t internal_format; 
        int32_t format;
        int32_t type;
        int32_t mag_filter;
        int32_t min_filter;
        int32_t wrap_s;
        int32_t wrap_t;
        glm::vec4 border_color;
    };

    struct subfrusta {
        glm::mat4 lightspace_view;
        glm::mat4 lightspace_projection;
        float far;
    };

    csm() = default;
    csm(size_t cascade_count, const shadowmap_config& config);

    csm(csm&& csm) noexcept;
    csm& operator=(csm&& csm) noexcept;

    void create(size_t cascade_count, const shadowmap_config& config) noexcept;

    void calculate_subfrustas(float fov, float aspect, float near, float far, const glm::mat4& view, const glm::vec3& light_direction, float z_mult) noexcept;

    void bind_for_writing(size_t cascade_index) const noexcept;
    void bind_for_reading(size_t first_slot) const noexcept;

    csm(const csm& other) = delete;
    csm& operator=(const csm& other) = delete;

private:
    std::vector<glm::vec4> _get_subfrusta_corners_world_space(const glm::mat4& view, const glm::mat4& projection) const noexcept;
    subfrusta _get_subfrusta_matrices(
        const glm::mat4& subfrusta_view, const glm::mat4& subfrusta_projection, const glm::vec3& light_direction, float z_mult) const noexcept;

public:
    framebuffer fbo;
    std::vector<texture_2d> shadowmaps;
    std::vector<subfrusta> subfrustas;

    struct data {
        uint32_t width;
        uint32_t height;
    } data;
};
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

    csm() = default;
    csm(size_t cascade_count, const shadowmap_config& config);

    csm(csm&& csm) noexcept;
    csm& operator=(csm&& csm) noexcept;

    void create(size_t cascade_count, const shadowmap_config& config) noexcept;

    void bind_for_writing(size_t cascade_index) const noexcept;
    void bind_for_reading(size_t first_slot) const noexcept;

    csm(const csm& other) = delete;
    csm& operator=(const csm& other) = delete;

    framebuffer fbo;
    std::vector<texture_2d> shadowmaps;

    struct data {
        uint32_t width;
        uint32_t height;
    } data;
};
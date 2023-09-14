#include "csm.hpp"

#include "debug.hpp"

#include <glad/glad.h>

csm::csm(size_t cascade_count, const shadowmap_config& config) {
    create(cascade_count, config);
}

void csm::create(size_t cascade_count, const shadowmap_config& config) noexcept {
    ASSERT(cascade_count > 0, "csm", "cascade count must be greater than 0");
    
    data.width = config.width;
    data.height = config.height;

    fbo.create();
    for (size_t i = 0; i < cascade_count; ++i) {
        texture_2d shadowmap(data.width, data.height, GL_TEXTURE_2D, 0, config.internal_format, config.format, config.type);
        shadowmap.set_parameter(GL_TEXTURE_WRAP_S, config.wrap_s);
        shadowmap.set_parameter(GL_TEXTURE_WRAP_T, config.wrap_t);
        shadowmap.set_parameter(GL_TEXTURE_MAG_FILTER, config.mag_filter);
        shadowmap.set_parameter(GL_TEXTURE_MIN_FILTER, config.min_filter);
        shadowmap.set_parameter(GL_TEXTURE_BORDER_COLOR, &config.border_color.r);
        shadowmaps.emplace_back(std::move(shadowmap));
    }

    fbo.attach(GL_DEPTH_ATTACHMENT, 0, shadowmaps[0]);

    fbo.set_draw_buffer(GL_NONE);
    fbo.set_read_buffer(GL_NONE);
}

void csm::bind_for_writing(size_t cascade_index) const noexcept {
    ASSERT(cascade_index < shadowmaps.size(), "csm", "invalid cascade index");
    fbo.attach(GL_DEPTH_ATTACHMENT, 0, shadowmaps[cascade_index]); 
}

void csm::bind_for_reading(size_t first_slot) const noexcept {
    for (const auto& shadowmap : shadowmaps) {
        shadowmap.bind(first_slot);
    }
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
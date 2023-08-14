#pragma once
#include <vector>
#include <string>

class cubemap {
public:
    struct config {
        config() = default;
        config(
            uint32_t face_width, uint32_t face_height, 
            uint32_t internal_format, uint32_t format, uint32_t type,
            uint32_t wrap_s, uint32_t wrap_t, uint32_t wrap_r, uint32_t min_filter, uint32_t mag_filter
        );

        uint32_t face_width = 0;
        uint32_t face_height = 0;

        uint32_t internal_format = 0;
        uint32_t format = 0;
        uint32_t type = 0;

        uint32_t wrap_s = 0;
        uint32_t wrap_t = 0;
        uint32_t wrap_r = 0;
        uint32_t min_filter = 0;
        uint32_t mag_filter = 0;
    };

public:
    cubemap() = default;
    cubemap(const config& config);
    cubemap(const std::vector<std::string>& faces, const config& config, bool flip_on_load = false);
    ~cubemap();

    void load(const std::vector<std::string>& faces, const config& config, bool flip_on_load = false) noexcept;
    void create(const config& config) noexcept;
    void destroy() noexcept;

    void bind(uint32_t unit = 0) const noexcept;
    void unbind() const noexcept;

    uint32_t get_id() const noexcept;
    uint32_t get_unit() const noexcept;
    const config& get_config_data() const noexcept;

    cubemap(cubemap&& cubemap);
    cubemap& operator=(cubemap&& cubemap) noexcept;

    cubemap(const cubemap& cubemap) = delete;
    cubemap& operator=(const cubemap& cubemap) = delete;

private:
    void _setup_tex_parametes(const config& config) const noexcept;
    uint32_t _get_channel_correct_format(uint32_t format, uint32_t channel_count) const noexcept;

private:
    struct cubemap_data {
        config config;
        uint32_t id = 0;
        uint32_t texture_unit = 0;
        uint32_t channel_count = 0;
    };

private:
    cubemap_data m_data;
};
#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>

class texture_2d {
public:
    enum class variety { NONE, DIFFUSE, SPECULAR, NORMAL, EMISSION };

    texture_2d() = default;
    texture_2d(const std::string &filepath, int32_t target, int32_t level, 
        int32_t internal_format, int32_t format, int32_t type, bool flip_on_load = true, variety variety = variety::NONE);
    texture_2d(uint32_t width, uint32_t height, int32_t target, int32_t level, 
        int32_t internal_format, int32_t format, int32_t type, void* pixels = nullptr, variety variety = variety::NONE);
    ~texture_2d();

    void load(const std::string &filepath, int32_t target, int32_t level, 
        int32_t internal_format, int32_t format, int32_t type, bool flip_on_load = true, variety variety = variety::NONE) noexcept;
    void create(uint32_t width, uint32_t height, int32_t target, int32_t level, 
        int32_t internal_format, int32_t format, int32_t type, void* pixels = nullptr, variety variety = variety::NONE) noexcept;
    void destroy() noexcept;

    void generate_mipmap() const noexcept;
    void set_tex_parameter(uint32_t pname, int32_t param) const noexcept;
    void set_tex_parameter(uint32_t pname, float param) const noexcept;
    void set_tex_parameter(uint32_t pname, const float* params) const noexcept;

    void bind(int32_t unit = -1) const noexcept;
    void unbind() const noexcept;

    uint32_t get_id() const noexcept;
    uint32_t get_unit() const noexcept;
    uint32_t get_target() const noexcept;
    uint32_t get_width() const noexcept;
    uint32_t get_height() const noexcept;
    variety get_variety() const noexcept;

    texture_2d(texture_2d&& texture);
    texture_2d& operator=(texture_2d&& texture) noexcept;

    texture_2d(const texture_2d& texture) = delete;
    texture_2d& operator=(const texture_2d& texture) = delete;
    
private:
    struct data {
        uint32_t id = 0;
        
        uint32_t target = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        
        uint32_t texture_unit = 0;

        variety variety = variety::NONE;
    };

private:
    static std::unordered_map<std::string, data> preloaded_textures;

private:
    data m_data;
};
#pragma once
#include "mesh.hpp"
#include "shader.hpp"
#include "texture.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <string>
#include <unordered_map>

class model  {
public:
    struct texture_config {
        int32_t target = 0;
        int32_t level = 0;
        int32_t internal_format = 0;
        int32_t format = 0;
        int32_t type = 0;
        int32_t min_filter = 0;
        int32_t mag_filter = 0;
        int32_t wrap_s = 0;
        int32_t wrap_t = 0;
        bool generate_mipmap = true;
        bool flip_on_load = true;
    };

public:
    model() = default;
    model(const std::string& filepath, const texture_config& config);
    
    void create(const std::string& filepath, const texture_config& config) noexcept;
    const std::vector<mesh>* get_meshes() const noexcept;

private:
    void _load_model(const std::string& filepath, const texture_config& config) noexcept;
    
    void _process_node(const texture_config& config, aiNode *ai_node, const aiScene *ai_scene) noexcept;
    
    mesh _process_mesh(const texture_config& config, aiMesh *ai_mesh, const aiScene *ai_scene) const noexcept;
    
    std::unordered_map<std::string, texture_2d> _load_material_texture_configs(const texture_config& config, 
        texture_2d::variety variety, aiMaterial *ai_mat, aiTextureType ai_type) const noexcept;

private:
    static std::unordered_map<std::string, std::vector<mesh>> preloaded_models;

private:
    std::vector<mesh>* m_meshes = nullptr;
    std::string m_directory;
};
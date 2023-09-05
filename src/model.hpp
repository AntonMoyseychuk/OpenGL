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
    model() = default;
    model(const std::string& filepath, 
        uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, bool flip_on_load);
    
    void create(const std::string& filepath, 
        uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, bool flip_on_load) noexcept;
    const std::vector<mesh>* get_meshes() const noexcept;

private:
    void _load_model(const std::string& filepath, 
        uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, bool flip_on_load) noexcept;
    
    void _process_node(uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, bool flip_on_load,
        aiNode *ai_node, const aiScene *ai_scene) noexcept;
    
    mesh _process_mesh(uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, bool flip_on_load,    
        aiMesh *ai_mesh, const aiScene *ai_scene) const noexcept;
    
    std::unordered_map<std::string, texture_2d> _load_material_texture_configs(uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, 
        uint32_t type, bool flip_on_load, texture_2d::variety variety, aiMaterial *ai_mat, aiTextureType ai_type) const noexcept;

private:
    static std::unordered_map<std::string, std::vector<mesh>> preloaded_models;

private:
    std::vector<mesh>* m_meshes = nullptr;
    std::string m_directory;
};
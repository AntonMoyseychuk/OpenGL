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
    model(const std::string& filepath, const texture::config& tex_config);
    
    void create(const std::string& filepath, const texture::config& tex_config) noexcept;
    const std::vector<mesh>* get_meshes() const noexcept;

private:
    void _load_model(const std::string& filepath, const texture::config& tex_config) noexcept;
    void _process_node(aiNode *ai_node, const aiScene *ai_scene, const texture::config& tex_config) noexcept;
    mesh _process_mesh(aiMesh *ai_mesh, const aiScene *ai_scene, const texture::config& tex_config) const noexcept;
    std::unordered_map<std::string, texture::config> _load_material_texture_configs(
        aiMaterial *ai_mat, aiTextureType ai_type, const texture::config& tex_config) const noexcept;

private:
    static std::unordered_map<std::string, std::vector<mesh>> preloaded_models;

private:
    std::vector<mesh>* m_meshes = nullptr;
    std::string m_directory;
};
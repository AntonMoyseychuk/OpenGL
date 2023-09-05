#include "model.hpp"

#include "log.hpp"

#include <filesystem>

std::unordered_map<std::string, std::vector<mesh>> model::preloaded_models;

model::model(const std::string &filepath, 
    uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, bool flip_on_load
) {
    create(filepath, target, level, internal_format, format, type, flip_on_load);
}

void model::create(const std::string &filepath, 
    uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, bool flip_on_load
) noexcept {
    _load_model(filepath, target, level, internal_format, format, type, flip_on_load);
}

const std::vector<mesh> *model::get_meshes() const noexcept {
    return m_meshes;
}

void model::_load_model(const std::string& filepath, 
    uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, bool flip_on_load
) noexcept {
    m_directory = std::filesystem::path(filepath).parent_path().u8string();
    
    if (preloaded_models.find(filepath) != preloaded_models.cend()) {
        m_meshes = &preloaded_models.at(filepath);
        return;
    }

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs);

    ASSERT(scene != nullptr && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode, "assimp error", importer.GetErrorString());

    m_meshes = &preloaded_models[filepath];
    _process_node(target, level, internal_format, format, type, flip_on_load, scene->mRootNode, scene);
}

void model::_process_node(
    uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, bool flip_on_load,
    aiNode *ai_node, const aiScene *ai_scene
) noexcept {
    for (size_t i = 0; i < ai_node->mNumMeshes; ++i) {
        aiMesh* mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
        m_meshes->push_back(_process_mesh(target, level, internal_format, format, type, flip_on_load, mesh, ai_scene));
    }

    for (size_t i = 0; i < ai_node->mNumChildren; ++i) {
        _process_node(target, level, internal_format, format, type, flip_on_load, ai_node->mChildren[i], ai_scene);
    }
}

mesh model::_process_mesh(
    uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, uint32_t type, bool flip_on_load,   
    aiMesh *ai_mesh, const aiScene *ai_scene
) const noexcept {
    std::vector<mesh::vertex> vertices;
    vertices.reserve(ai_mesh->mNumVertices);

    std::vector<uint32_t> indices;

    for(size_t i = 0; i < ai_mesh->mNumVertices; ++i) {
        mesh::vertex vertex;
        vertex.position = glm::vec3(ai_mesh->mVertices[i].x, ai_mesh->mVertices[i].y, ai_mesh->mVertices[i].z);
        vertex.normal = glm::vec3(ai_mesh->mNormals[i].x, ai_mesh->mNormals[i].y, ai_mesh->mNormals[i].z);
        
        if (ai_mesh->mTextureCoords[0]) {
            vertex.texcoord = glm::vec2(ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y);
        } else {
            vertex.texcoord = glm::vec2(0.0f);
        }

        vertices.emplace_back(vertex);
    }
    
    for (size_t i = 0; i < ai_mesh->mNumFaces; ++i) {
        const aiFace& face = ai_mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }
    
    std::unordered_map<std::string, texture_2d> textures;
    if(ai_mesh->mMaterialIndex >= 0) {
        aiMaterial *material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];

        for (auto& t : _load_material_texture_configs(target, level, internal_format, format, type,
            flip_on_load, texture_2d::variety::DIFFUSE, material, aiTextureType_DIFFUSE)) {
            textures.insert(std::move(t));
        }
        
        for (auto& t : _load_material_texture_configs(target, level, internal_format, format, type,
            flip_on_load, texture_2d::variety::SPECULAR, material, aiTextureType_DIFFUSE)) {
            textures.insert(std::move(t));
        }

        for (auto& t : _load_material_texture_configs(target, level, internal_format, format, type,
            flip_on_load, texture_2d::variety::NORMAL, material, aiTextureType_DIFFUSE)) {
            textures.insert(std::move(t));
        }

        for (auto& t : _load_material_texture_configs(target, level, internal_format, format, type,
            flip_on_load, texture_2d::variety::HEIGHT, material, aiTextureType_DIFFUSE)) {
            textures.insert(std::move(t));
        }
    }

    mesh mesh;
    mesh.create(vertices, indices);

    for (auto& texture : textures) {
        mesh.add_texture(std::move(texture.second));
    }

    return mesh;
}

std::unordered_map<std::string, texture_2d> model::_load_material_texture_configs(
    uint32_t target, uint32_t level, uint32_t internal_format, uint32_t format, 
    uint32_t type, bool flip_on_load, texture_2d::variety variety, 
    aiMaterial *ai_mat, aiTextureType ai_type
) const noexcept {
    std::unordered_map<std::string, texture_2d> textures;
    textures.reserve(ai_mat->GetTextureCount(ai_type));

    for(size_t i = 0; i < ai_mat->GetTextureCount(ai_type); ++i) {
        aiString texture_name;
        ai_mat->GetTexture(ai_type, i, &texture_name);

        const std::string filepath = m_directory + "\\" + texture_name.C_Str();
        texture_2d texture(filepath, target, level, internal_format, format, type, flip_on_load, variety);
        textures[filepath] = std::move(texture);
    }

    return std::move(textures);
}

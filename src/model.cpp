#include "model.hpp"

#include "log.hpp"

#include <filesystem>

std::unordered_map<std::string, std::vector<mesh>> model::preloaded_models;

model::model(const std::string &filepath) {
    create(filepath);
}

void model::create(const std::string &filepath) noexcept {
    _load_model(filepath);
}

const std::vector<mesh> *model::get_meshes() const noexcept {
    return m_meshes;
}

void model::_load_model(const std::string& filepath) noexcept {
    m_directory = std::filesystem::path(filepath).parent_path().u8string();
    
    if (preloaded_models.find(filepath) != preloaded_models.cend()) {
        m_meshes = &preloaded_models.at(filepath);
        return;
    }

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs);

    ASSERT(scene != nullptr && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode, "assimp error", importer.GetErrorString());

    m_meshes = &preloaded_models[filepath];
    _process_node(scene->mRootNode, scene);
}

void model::_process_node(aiNode *ai_node, const aiScene *ai_scene) noexcept {
    for (size_t i = 0; i < ai_node->mNumMeshes; ++i) {
        aiMesh* mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
        m_meshes->push_back(_process_mesh(mesh, ai_scene));
    }

    for (size_t i = 0; i < ai_node->mNumChildren; ++i) {
        _process_node(ai_node->mChildren[i], ai_scene);
    }
}

mesh model::_process_mesh(aiMesh *ai_mesh, const aiScene *ai_scene) const noexcept {
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
    
    std::unordered_map<std::string, texture::config> textures;
    if(ai_mesh->mMaterialIndex >= 0) {
        aiMaterial *material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
        
        for (const auto& t : _load_material_texture_configs(material, aiTextureType_DIFFUSE, texture::type::DIFFUSE)) {
            textures.insert(t);
        }
        
        for (const auto& t : _load_material_texture_configs(material, aiTextureType_SPECULAR, texture::type::SPECULAR)) {
            textures.insert(t);
        }

        for (const auto& t : _load_material_texture_configs(material, aiTextureType_NORMALS, texture::type::NORMAL)) {
            textures.insert(t);
        }
    }

    mesh mesh;
    mesh.create(vertices, indices, textures);

    return mesh;
}

std::unordered_map<std::string, texture::config> model::_load_material_texture_configs(
    aiMaterial *ai_mat, aiTextureType ai_type, texture::type texture_type) const noexcept 
{
    std::unordered_map<std::string, texture::config> texture_configs;
    texture_configs.reserve(ai_mat->GetTextureCount(ai_type));

    const texture::config config(GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_FALSE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true, texture_type);

    for(size_t i = 0; i < ai_mat->GetTextureCount(ai_type); ++i) {
        aiString texture_name;
        ai_mat->GetTexture(ai_type, i, &texture_name);
        texture_configs.insert(std::make_pair(m_directory + "\\" + texture_name.C_Str(), config));
    }

    return texture_configs;
}

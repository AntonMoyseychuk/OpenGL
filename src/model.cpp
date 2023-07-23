#include "model.hpp"

#include "log.hpp"

#include <filesystem>

std::unordered_map<std::string, std::vector<mesh>> model::preloaded_models;

model::model(const std::string &filepath) {
    create(filepath);
}

void model::create(const std::string &filepath) const noexcept {
    _load_model(filepath);
}

void model::draw(const shader &shader) const noexcept {
    if (m_meshes == nullptr) {
        LOG_WARN("render", "m_meshes == nullptr");
        return;
    }

    for (size_t i = 0; i < m_meshes->size(); ++i) {
        m_meshes->at(i).draw(shader);
    }
}

void model::_load_model(const std::string& filepath) const noexcept {
    model* self = const_cast<model*>(this);

    self->m_directory = std::filesystem::path(filepath).parent_path().u8string();
    
    if (preloaded_models.find(filepath) != preloaded_models.cend()) {
        self->m_meshes = &preloaded_models.at(filepath);
        return;
    }

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs);

    ASSERT(scene != nullptr && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode, "assimp error", importer.GetErrorString());

    self->m_meshes = &preloaded_models[filepath];
    _process_node(scene->mRootNode, scene);
}

void model::_process_node(aiNode *ai_node, const aiScene *ai_scene) const noexcept {
    model* self = const_cast<model*>(this);
    
    for (size_t i = 0; i < ai_node->mNumMeshes; ++i) {
        aiMesh* mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
        self->m_meshes->push_back(_process_mesh(mesh, ai_scene));
    }

    for (size_t i = 0; i < ai_node->mNumChildren; ++i) {
        _process_node(ai_node->mChildren[i], ai_scene);
    }
}

mesh model::_process_mesh(aiMesh *ai_mesh, const aiScene *ai_scene) const noexcept {
    std::vector<mesh::vertex> vertices;
    vertices.reserve(ai_mesh->mNumVertices);

    std::vector<uint32_t> indices;
    std::vector<texture> textures;

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
    
    if(ai_mesh->mMaterialIndex >= 0) {
        aiMaterial *material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
        std::vector<texture> diffuse_maps = _load_material_textures(material, aiTextureType_DIFFUSE, texture::type::DIFFUSE);
        textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());
        
        std::vector<texture> specular_maps = _load_material_textures(material, aiTextureType_SPECULAR, texture::type::SPECULAR);
        textures.insert(textures.end(), specular_maps.begin(), specular_maps.end());

        std::vector<texture> emission_maps = _load_material_textures(material, aiTextureType_EMISSIVE, texture::type::EMISSION);
        textures.insert(textures.end(), emission_maps.begin(), emission_maps.end());

        std::vector<texture> normal_maps = _load_material_textures(material, aiTextureType_NORMALS, texture::type::NORMAL);
        textures.insert(textures.end(), normal_maps.begin(), normal_maps.end());
    }

    mesh mesh;
    mesh.create(vertices, indices, textures);

    return mesh;
}

std::vector<texture> model::_load_material_textures(aiMaterial *ai_mat, aiTextureType ai_type, texture::type texture_type) const noexcept {
    std::vector<texture> textures;
    textures.reserve(ai_mat->GetTextureCount(ai_type));

    const texture::config config(GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true, texture_type);

    for(size_t i = 0; i < ai_mat->GetTextureCount(ai_type); ++i) {
        aiString texture_name;
        ai_mat->GetTexture(ai_type, i, &texture_name);
        
        texture texture;
        texture.create(m_directory + "\\" + texture_name.C_Str(), config);
        
        textures.emplace_back(texture);
    }

    return textures;
}

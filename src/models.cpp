#include "includes/engine.h"

// TODO:: add model loading via assimp


std::optional<MeshRenderableVariant> Model::loadFromAssetsFolder(const std::string name, const unsigned int importerFlags, const bool useFirstChildAsRoot)
{
    return Model::load((Engine::getAppPath(APP_PATHS::MODELS) / name).string(), importerFlags, useFirstChildAsRoot);
}

std::optional<MeshRenderableVariant> Model::load(const std::string name, const unsigned int importerFlags, const bool useFirstChildAsRoot)
{
    Assimp::Importer importer;

    const aiScene * scene = importer.ReadFile(name.c_str(), importerFlags);


    if (scene == nullptr) {
        logError(importer.GetErrorString());
        return std::nullopt;
    }

    if (!scene->HasMeshes()) {
        logError("Model does not contain meshes");
        return std::nullopt;
    }

    aiNode * root = useFirstChildAsRoot ? scene->mRootNode->mChildren[0] : scene->mRootNode;
    const auto & parentPath = std::filesystem::path(name).parent_path();

    //if (scene->HasAnimations()) {
        // TODO: implement later
    //} else {
        auto modelMesh = std::make_unique<ModelMeshGeometry>();
        Model::processModelNode(root, scene, modelMesh, parentPath);

        modelMesh->bbox = Helper::createBoundingBoxFromMinMax(modelMesh->bbox.min, modelMesh->bbox.max);

        auto modelMeshRenderable = std::make_unique<ModelMeshRenderable>(modelMesh);
        return GlobalRenderableStore::INSTANCE()->registerRenderable<ModelMeshRenderable>(modelMeshRenderable);
    //}

    return std::nullopt;
}

void Model::processModelNode(const aiNode * node, const aiScene * scene, std::unique_ptr<ModelMeshGeometry> & modelMeshGeom, const std::filesystem::path & parentPath)
{
    for(unsigned int i=0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        Model::processModelMesh(mesh, scene, modelMeshGeom, parentPath);
    }

    for(unsigned int i=0; i<node->mNumChildren; i++) {
        Model::processModelNode(node->mChildren[i], scene, modelMeshGeom, parentPath);
    }
}

void Model::processModelMesh(const aiMesh * mesh, const aiScene * scene, std::unique_ptr<ModelMeshGeometry> & modelMeshGeom, const std::filesystem::path & parentPath)
{
    ModelMeshIndexed modelMesh;

    /**
     * MESH COLOR / TEXTURE
     */
    if (scene->HasMaterials()) {
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::unique_ptr<aiColor4D> diffuse(new aiColor4D());
        if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, diffuse.get()) == aiReturn_SUCCESS) {
            glm::vec4 diffuseVec4 = { diffuse->r, diffuse->g, diffuse->b, diffuse->a };
            if (diffuseVec4 != glm::vec4(0.0f)) modelMesh.material.color = diffuseVec4;
        };

        float shiny = 0.0f;
        aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shiny);
        if (shiny == 0.0f) shiny = 5.0f;

        std::unique_ptr<aiColor4D> specular(new aiColor4D());
        if (aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, specular.get()) == aiReturn_SUCCESS) {
            glm::vec3 specularVec3 { specular->r, specular->g, specular->b };
            if (specularVec3 != glm::vec3(0.0f)) modelMesh.material.specularColor = glm::vec4 {specularVec3.r, specularVec3.b, specularVec3.g, shiny };
        };

        Model::processMeshTexture(material, scene, modelMesh.textures, parentPath);
    }

    /**
     *  VERTICES
     */
    if (mesh->mNumVertices > 0) {
        modelMesh.vertices.reserve(mesh->mNumVertices);
    }

    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        ModelVertex vertex;

        vertex.position = glm::vec3 { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
        vertex.normal = glm::vec3(0.0f);
        vertex.uv = glm::vec2 { 0.0f, 0.0f };

        if (mesh->HasNormals()) {
            vertex.normal = glm::normalize(glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
        }

        if(mesh->HasTextureCoords(0)) {
            vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }

        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }

        modelMeshGeom->bbox.min.x = std::min(vertex.position.x, modelMeshGeom->bbox.min.x);
        modelMeshGeom->bbox.min.y = std::min(vertex.position.y, modelMeshGeom->bbox.min.y);
        modelMeshGeom->bbox.min.z = std::min(vertex.position.z, modelMeshGeom->bbox.min.z);

        modelMeshGeom->bbox.max.x = std::max(vertex.position.x, modelMeshGeom->bbox.max.x);
        modelMeshGeom->bbox.max.y = std::max(vertex.position.y, modelMeshGeom->bbox.max.y);
        modelMeshGeom->bbox.max.z = std::max(vertex.position.z, modelMeshGeom->bbox.max.z);

        modelMesh.vertices.push_back(vertex);
    }

     /**
     *  INDICES
     */
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace face = mesh->mFaces[i];

        for(unsigned int j = 0; j < face.mNumIndices; j++) {
            modelMesh.indices.push_back(face.mIndices[j]);
        }
    }

    modelMeshGeom->meshes.emplace_back(std::move(modelMesh));
}

void Model::processMeshTexture(const aiMaterial * mat, const aiScene * scene, TextureInformation & meshTextureInfo, const std::filesystem::path & parentPath)
{
    if (mat->GetTextureCount(aiTextureType_AMBIENT) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_AMBIENT, 0, &str);
        if (str.length > 0) Model::correctTexturePath(str.data);

        std::string textureName(str.C_Str());
        std::string textureLocation = (parentPath / std::filesystem::path(textureName)).string();

        const aiTexture * texture = scene != nullptr ? scene->GetEmbeddedTexture(textureName.c_str()) : nullptr;
        if (texture != nullptr) {
            const std::string embeddedModelFile = Model::saveEmbeddedModelTexture(texture);
            if (!embeddedModelFile.empty()) textureLocation = embeddedModelFile;
        }

        meshTextureInfo.ambientTexture = GlobalTextureStore::INSTANCE()->getOrAddTexture(textureName, textureLocation);
    }

    if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);
        if (str.length > 0) Model::correctTexturePath(str.data);

        std::string textureName(str.C_Str());
        std::string textureLocation = (parentPath / std::filesystem::path(textureName)).string();

        const aiTexture * texture = scene != nullptr ? scene->GetEmbeddedTexture(textureName.c_str()) : nullptr;
        if (texture != nullptr) {
            const std::string embeddedModelFile = Model::saveEmbeddedModelTexture(texture);
            if (!embeddedModelFile.empty()) textureLocation = embeddedModelFile;
        }

        meshTextureInfo.diffuseTexture = GlobalTextureStore::INSTANCE()->getOrAddTexture(textureName, textureLocation);
    }

    if (mat->GetTextureCount(aiTextureType_SPECULAR) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_SPECULAR, 0, &str);
        if (str.length > 0) Model::correctTexturePath(str.data);

        std::string textureName(str.C_Str());
        std::string textureLocation = (parentPath / std::filesystem::path(textureName)).string();

        const aiTexture * texture = scene != nullptr ? scene->GetEmbeddedTexture(textureName.c_str()) : nullptr;
        if (texture != nullptr) {
            const std::string embeddedModelFile = Model::saveEmbeddedModelTexture(texture);
            if (!embeddedModelFile.empty()) textureLocation = embeddedModelFile;
        }

        meshTextureInfo.specularTexture = GlobalTextureStore::INSTANCE()->getOrAddTexture(textureName, textureLocation);
    }

    if (mat->GetTextureCount(aiTextureType_HEIGHT) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_HEIGHT, 0, &str);
        if (str.length > 0) Model::correctTexturePath(str.data);

        std::string textureName(str.C_Str());
        std::string textureLocation = (parentPath / std::filesystem::path(textureName)).string();

        const aiTexture * texture = scene != nullptr ? scene->GetEmbeddedTexture(textureName.c_str()) : nullptr;
        if (texture != nullptr) {
            const std::string embeddedModelFile = Model::saveEmbeddedModelTexture(texture);
            if (!embeddedModelFile.empty()) textureLocation = embeddedModelFile;
        }

        meshTextureInfo.normalTexture = GlobalTextureStore::INSTANCE()->getOrAddTexture(textureName, textureLocation);
    } else if (mat->GetTextureCount(aiTextureType_NORMALS) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_NORMALS, 0, &str);
        if (str.length > 0) Model::correctTexturePath(str.data);

        std::string textureName(str.C_Str());
        std::string textureLocation = (parentPath / std::filesystem::path(textureName)).string();

        const aiTexture * texture = scene != nullptr ? scene->GetEmbeddedTexture(textureName.c_str()) : nullptr;
        if (texture != nullptr) {
            const std::string embeddedModelFile = Model::saveEmbeddedModelTexture(texture);
            if (!embeddedModelFile.empty()) textureLocation = embeddedModelFile;
        }

        meshTextureInfo.normalTexture = GlobalTextureStore::INSTANCE()->getOrAddTexture(textureName, textureLocation);
    }
}

void Model::correctTexturePath(char * path) {
    int index = 0;

    while(path[index] == '\0') index++;

    if(index != 0) {
        int i = 0;
        while(path[i + index] != '\0') {
            path[i] = path[i + index];
            i++;
        }
        path[i] = '\0';
    }
}

std::string Model::saveEmbeddedModelTexture(const aiTexture * texture) {
    if (texture == nullptr) return "";

    if (texture->mHeight > 0) {
        logError("Embedded Non Compressed Textures are not Supported");
        return "";
    }

    const std::string textureName = std::string(texture->mFilename.C_Str());
    if (textureName.empty()) return "";

    const std::filesystem::path textureFile = Engine::getAppPath(TEMP) / textureName;

    std::ofstream tmpFile(textureFile, std::ios::out | std::ios::binary);
    tmpFile.write((char *) texture->pcData, texture->mWidth);
    tmpFile.close();

    return textureFile.string();
}



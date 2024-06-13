#include "includes/engine.h"

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

    if (scene->HasAnimations()) {
        auto modelMesh = std::make_unique<AnimatedModelMeshGeometry>();
        modelMesh->joints.reserve(MAX_JOINTS);

        Model::processModelNode(root, scene, modelMesh, parentPath);

        modelMesh->bbox = Helper::createBoundingBoxFromMinMax(modelMesh->bbox.min, modelMesh->bbox.max);

        if (!modelMesh->jointIndexByName.empty()) {
            modelMesh->joints.resize(modelMesh->jointIndexByName.size());

            const aiMatrix4x4 & trans = root->mTransformation;
            glm::mat4 rootNodeTransform = {
                { trans.a1, trans.b1, trans.c1, trans.d1 },
                { trans.a2, trans.b2, trans.c2, trans.d2 },
                { trans.a3, trans.b3, trans.c3, trans.d3 },
                { trans.a4, trans.b4, trans.c4, trans.d4 }
            };

            NodeInformation rootNode = { };
            rootNode.name = root->mName.C_Str();

            rootNode.transformation = rootNodeTransform;
            modelMesh->rootNode = rootNode;

            modelMesh->rootInverseTransformation = glm::inverse(rootNodeTransform);
            Model::processJoints(root, modelMesh->rootNode, -1, modelMesh, true);

            Model::processAnimations(scene, modelMesh);
        }

        auto modelMeshRenderable = std::make_unique<AnimatedModelMeshRenderable>(modelMesh);
        return GlobalRenderableStore::INSTANCE()->registerRenderable<AnimatedModelMeshRenderable>(modelMeshRenderable);
    } else {
        auto modelMesh = std::make_unique<ModelMeshGeometry>();
        Model::processModelNode(root, scene, modelMesh, parentPath);

        modelMesh->bbox = Helper::createBoundingBoxFromMinMax(modelMesh->bbox.min, modelMesh->bbox.max);

        auto modelMeshRenderable = std::make_unique<ModelMeshRenderable>(modelMesh);
        return GlobalRenderableStore::INSTANCE()->registerRenderable<ModelMeshRenderable>(modelMeshRenderable);
    }

    return std::nullopt;
}

void Model::processModelNode(const aiNode * node, const aiScene * scene, std::unique_ptr<ModelMeshGeometry> & modelMeshGeom, const std::filesystem::path & parentPath)
{
    for(unsigned int i=0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        Model::processModelMesh<ModelMeshGeometry,ModelMeshIndexed>(mesh, scene, modelMeshGeom, parentPath);
    }

    for(unsigned int i=0; i<node->mNumChildren; i++) {
        Model::processModelNode(node->mChildren[i], scene, modelMeshGeom, parentPath);
    }
}

void Model::processModelNode(const aiNode * node, const aiScene * scene, std::unique_ptr<AnimatedModelMeshGeometry> & modelMeshGeom, const std::filesystem::path & parentPath)
{
    uint32_t vertexOffset = 0;

    for(unsigned int i=0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        Model::processModelMesh<AnimatedModelMeshGeometry,AnimatedModelMeshIndexed>(mesh, scene, modelMeshGeom, parentPath);

        const auto additionalVertices = modelMeshGeom->meshes.empty() ? 0 : modelMeshGeom->meshes[modelMeshGeom->meshes.size()-1].vertices.size();
        for (int i=0;i<additionalVertices;i++) modelMeshGeom->vertexJointInfo.emplace_back(VertexJointInfo {});

        Model::processModelMeshAnimation(mesh, modelMeshGeom, vertexOffset);

        vertexOffset += additionalVertices;
    }

    for(unsigned int i=0; i<node->mNumChildren; i++) {
        Model::processModelNode(node->mChildren[i], scene, modelMeshGeom, parentPath);
    }
}

template <typename G, typename M>
void Model::processModelMesh(const aiMesh * mesh, const aiScene * scene, std::unique_ptr<G> & modelMeshGeom, const std::filesystem::path & parentPath) {
    M modelMesh;

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
        modelMesh.material.shininess = shiny;

        std::unique_ptr<aiColor4D> specular(new aiColor4D());
        if (aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, specular.get()) == aiReturn_SUCCESS) {
            glm::vec3 specularVec3 { specular->r, specular->g, specular->b };
            if (specularVec3 != glm::vec3(0.0f)) modelMesh.material.specularColor = {specularVec3.r, specularVec3.g, specularVec3.b};
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

        modelMesh.vertices.emplace_back(vertex);
    }

     /**
     *  INDICES
     */
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace face = mesh->mFaces[i];

        for(unsigned int j = 0; j < face.mNumIndices; j++) {
            modelMesh.indices.emplace_back(face.mIndices[j]);
        }
    }

    modelMeshGeom->meshes.emplace_back(std::move(modelMesh));
}

void Model::processModelMeshAnimation(const aiMesh * mesh, std::unique_ptr<AnimatedModelMeshGeometry> & animatedModelMeshGeometry, uint32_t vertexOffset) {
    for(unsigned int i = 0; i < mesh->mNumBones; i++) {
        const aiBone * bone = mesh->mBones[i];

        if (bone->mName.length == 0) {
            continue;
        }

        const std::string boneName = std::string(bone->mName.C_Str(), bone->mName.length);
        uint32_t boneIndex = animatedModelMeshGeometry->jointIndexByName.size();

        if (!animatedModelMeshGeometry->jointIndexByName.contains(boneName)) {
            animatedModelMeshGeometry->jointIndexByName[boneName] = boneIndex;

            JointInformation jointInfo {};
            jointInfo.name = boneName;
            jointInfo.offsetMatrix = glm::mat4 {
                {
                    bone->mOffsetMatrix.a1, bone->mOffsetMatrix.b1, bone->mOffsetMatrix.c1, bone->mOffsetMatrix.d1
                },
                {
                    bone->mOffsetMatrix.a2, bone->mOffsetMatrix.b2, bone->mOffsetMatrix.c2, bone->mOffsetMatrix.d2
                },
                {
                    bone->mOffsetMatrix.a3, bone->mOffsetMatrix.b3, bone->mOffsetMatrix.c3, bone->mOffsetMatrix.d3
                },
                {
                    bone->mOffsetMatrix.a4, bone->mOffsetMatrix.b4, bone->mOffsetMatrix.c4, bone->mOffsetMatrix.d4
                }
            };

            animatedModelMeshGeometry->joints.push_back(jointInfo);

            for(unsigned int j = 0; j < bone->mNumWeights; j++) {
                uint32_t vertexId = bone->mWeights[j].mVertexId;
                float jointWeight = bone->mWeights[j].mWeight;

                VertexJointInfo & jointInfo = animatedModelMeshGeometry->vertexJointInfo[vertexOffset + vertexId];
                Model::addVertexJointInfo(boneIndex, jointWeight, jointInfo);
            }
        } else {
            boneIndex = animatedModelMeshGeometry->jointIndexByName[boneName];

            for(unsigned int j = 0; j < bone->mNumWeights; j++) {
                uint32_t vertexId = bone->mWeights[j].mVertexId;
                float jointWeight = bone->mWeights[j].mWeight;

                VertexJointInfo & jointInfo = animatedModelMeshGeometry->vertexJointInfo[vertexOffset + vertexId];
                Model::addVertexJointInfo(boneIndex, jointWeight, jointInfo);
            }
        }
    }
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

void Model::addVertexJointInfo(const uint32_t jointIndex, float jointWeight,VertexJointInfo & jointInfo)
{
    if (jointWeight <= 0.0f) return;

    if (jointInfo.weights.x <= 0.0f) {
        jointInfo.vertexIds.x = jointIndex;
        jointInfo.weights.x = jointWeight;
        return;
    }

    if (jointInfo.weights.y <= 0.0f) {
        jointInfo.vertexIds.y = jointIndex;
        jointInfo.weights.y = jointWeight;
        return;
    }

    if (jointInfo.weights.z <= 0.0f) {
        jointInfo.vertexIds.z= jointIndex;
        jointInfo.weights.z = jointWeight;
        return;
    }

    if (jointInfo.weights.w <= 0.0f) {
        jointInfo.vertexIds.w = jointIndex;
        jointInfo.weights.w = jointWeight;
    }
}

void Model::processJoints(const aiNode * node, NodeInformation & parentNode, int32_t parentIndex, std::unique_ptr<AnimatedModelMeshGeometry> & animatedModelMeshGeometry, bool isRoot) {
    int32_t childIndex = -1;

    const std::string nodeName = node->mName.C_Str();

    const aiMatrix4x4 & trans = node->mTransformation;
    const glm::mat4 nodeTransformation = glm::mat4 {
        { trans.a1, trans.b1, trans.c1, trans.d1 },
        { trans.a2, trans.b2, trans.c2, trans.d2 },
        { trans.a3, trans.b3, trans.c3, trans.d3 },
        { trans.a4, trans.b4, trans.c4, trans.d4 }
    };

    if (!nodeName.empty() && animatedModelMeshGeometry->jointIndexByName.contains(nodeName)) {

        childIndex = animatedModelMeshGeometry->jointIndexByName[nodeName];

        JointInformation & joint = animatedModelMeshGeometry->joints[childIndex];
        joint.nodeTransformation = nodeTransformation;

        if (parentIndex != -1) {
            JointInformation & parentJoint = animatedModelMeshGeometry->joints[parentIndex];
            parentJoint.children.push_back(childIndex);
        }
    }

    if (!isRoot) {
        NodeInformation childNode = { };
        childNode.name = nodeName;
        childNode.transformation =  nodeTransformation;
        parentNode.children.push_back(childNode);
    }

    for (unsigned int i=0; i<node->mNumChildren; i++) {
        Model::processJoints(node->mChildren[i], isRoot ? parentNode : parentNode.children[parentNode.children.size()-1], childIndex, animatedModelMeshGeometry);
    }
}

void Model::processAnimations(const aiScene *scene, std::unique_ptr<AnimatedModelMeshGeometry> & animatedModelMeshGeometry) {
    for(unsigned int i=0; i < scene->mNumAnimations; i++) {
        const aiAnimation * anim = scene->mAnimations[i];

        std::string animName = "anim" + std::to_string(animatedModelMeshGeometry->animations.size());
        if (anim->mName.length > 0) {
            animName = anim->mName.C_Str();
        }

        AnimationInformation animInfo {};
        animInfo.duration = anim->mDuration;
        animInfo.ticksPerSecond = anim->mTicksPerSecond;

        for(unsigned int j=0; j < anim->mNumChannels; j++) {
            const aiNodeAnim * details = anim->mChannels[j];

            AnimationDetails animDetails {};
            animDetails.name = details->mNodeName.C_Str();

            for(unsigned int k=0; k < details->mNumPositionKeys; k++) {
                const aiVectorKey & posKeys = details->mPositionKeys[k];
                AnimationDetailsEntry translationEntry {};
                translationEntry.time = posKeys.mTime;
                translationEntry.translation = glm::vec3(posKeys.mValue[0], posKeys.mValue[1], posKeys.mValue[2]);
                animDetails.positions.push_back(translationEntry);
            }

            for(unsigned int k=0; k < details->mNumRotationKeys; k++) {
                const aiQuatKey & rotKeys = details->mRotationKeys[k];
                const aiQuaternion rotQuat = rotKeys.mValue;

                const glm::mat4 rotMatrix = glm::toMat4(glm::quat(rotQuat.w, rotQuat.x, rotQuat.y, rotQuat.z));
                AnimationDetailsEntry rotationEntry {};
                rotationEntry.time = rotKeys.mTime;
                rotationEntry.rotation = rotMatrix;
                animDetails.rotations.push_back(rotationEntry);
            }

            for(unsigned int k=0; k < details->mNumScalingKeys; k++) {
                const aiVectorKey & scaleKeys = details->mScalingKeys[k];
                AnimationDetailsEntry scaleEntry {};
                scaleEntry.time = scaleKeys.mTime;
                scaleEntry.scaling = glm::vec3(scaleKeys.mValue[0], scaleKeys.mValue[1], scaleKeys.mValue[2]);
                animDetails.scalings.push_back(scaleEntry);
            }

            animInfo.details.push_back(animDetails);
        }

        animatedModelMeshGeometry->animations[animName] = animInfo;
    }
}



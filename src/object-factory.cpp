#include "includes/server.h"

PhysicsObject * ObjectFactory::loadModel(const std::string modelFileLocation, const std::string id, const unsigned int importerFlags, const bool useFirstChildAsRoot)
{
    Assimp::Importer importer;

    unsigned int flags = 0 | aiProcess_FlipUVs | aiProcess_CalcTangentSpace;
    if (importerFlags != 0) flags |= importerFlags;

    const aiScene * scene = importer.ReadFile(modelFileLocation.c_str(), flags);

    if (scene == nullptr) {
        logError(importer.GetErrorString());
        return nullptr;
    }

    if (!scene->HasMeshes()) {
        logError("Model does not contain meshes");
        return nullptr;
    }

    if (!id.empty() && GlobalPhysicsObjectStore::INSTANCE()->getObjectById<PhysicsObject>(id) != nullptr) {
        logError("Object with id '" + id + "' already exists!");
        return nullptr;
    }

    const std::string objectId = id.empty() ? "object-" + std::to_string(ObjectFactory::getNextRunningId()) : id;

    aiNode * root = useFirstChildAsRoot ? scene->mRootNode->mChildren[0] : scene->mRootNode;
    const auto & parentPath = std::filesystem::path(modelFileLocation).parent_path();

    if (scene->HasAnimations()) {
        auto newPhysicsObject = std::make_unique<PhysicsObject>(objectId, MODEL);
        newPhysicsObject->reserveJoints();
        ObjectFactory::processModelNode(root, scene, newPhysicsObject, parentPath);
        newPhysicsObject->populateJoints(scene, root);
        return GlobalPhysicsObjectStore::INSTANCE()->registerObject<PhysicsObject>(newPhysicsObject);
    } else {
        auto newPhysicsObject = std::make_unique<PhysicsObject>(objectId, MODEL);
        ObjectFactory::processModelNode(root, scene, newPhysicsObject, parentPath);
        return GlobalPhysicsObjectStore::INSTANCE()->registerObject<PhysicsObject>(newPhysicsObject);
    }

    return nullptr;
}

void ObjectFactory::processModelNode(const aiNode * node, const aiScene * scene, std::unique_ptr<PhysicsObject> & physicsObject, const std::filesystem::path & parentPath)
{
    for(unsigned int i=0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        uint32_t vertexOffset = 0;
        for (auto & prevMesh : physicsObject->getMeshes()) {
            vertexOffset += prevMesh.vertices.size();
        }

        ObjectFactory::processModelMesh(mesh, scene, physicsObject, parentPath);
        ObjectFactory::processModelMeshAnimation(mesh, physicsObject, vertexOffset);
    }

    for(unsigned int i=0; i<node->mNumChildren; i++) {
        ObjectFactory::processModelNode(node->mChildren[i], scene, physicsObject, parentPath);
    }
}

void ObjectFactory::processModelMesh(const aiMesh * mesh, const aiScene * scene, std::unique_ptr<PhysicsObject> & physicsObject, const std::filesystem::path & parentPath)
{
    if (mesh->mNumVertices == 0) return;

    Mesh m;
    m.vertices.reserve(mesh->mNumVertices);

    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        vertex.position = glm::vec3 { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
        vertex.normal = glm::vec3(0.0f);

        if (mesh->HasNormals()) {
            vertex.normal = glm::normalize(glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
        }

        physicsObject->updateBboxWithVertex(vertex);

        m.vertices.emplace_back(vertex);

        if (scene->HasAnimations()) physicsObject->addVertexJointInfo({ {0, 0, 0, 0}, {0.0f, 0.0f, 0.0f, 0.0f} });
    }

    physicsObject->addMesh(m);
}

void ObjectFactory::processModelMeshAnimation(const aiMesh * mesh, std::unique_ptr<PhysicsObject> & physicsObject, uint32_t vertexOffset)
{
    for(unsigned int i = 0; i < mesh->mNumBones; i++) {
        const aiBone * bone = mesh->mBones[i];

        if (bone->mName.length == 0) {
            continue;
        }

        const std::string boneName = std::string(bone->mName.C_Str(), bone->mName.length);

        std::optional<uint32_t> boneIndex = physicsObject->getJointIndexByName(boneName);

        if (!boneIndex.has_value()) {
            physicsObject->updateJointIndexByName(boneName, std::nullopt);

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

            physicsObject->addJointInformation(jointInfo);

            for(unsigned int j = 0; j < bone->mNumWeights; j++) {
                uint32_t vertexId = bone->mWeights[j].mVertexId;
                float jointWeight = bone->mWeights[j].mWeight;

                boneIndex = physicsObject->getJointIndexByName(boneName);
                physicsObject->updateVertexJointInfo(vertexOffset + vertexId, boneIndex.value(), jointWeight);
            }
        } else {
            boneIndex = physicsObject->getJointIndexByName(boneName);

            for(unsigned int j = 0; j < bone->mNumWeights; j++) {
                uint32_t vertexId = bone->mWeights[j].mVertexId;
                float jointWeight = bone->mWeights[j].mWeight;

                physicsObject->updateVertexJointInfo(vertexOffset + vertexId, boneIndex.value(), jointWeight);
            }
        }
    }
}

PhysicsObject * ObjectFactory::loadSphere(const std::string id)
{
    if (!id.empty() && GlobalPhysicsObjectStore::INSTANCE()->getObjectById<PhysicsObject>(id) != nullptr) {
        logError("Object with id '" + id + "' already exists!");
        return nullptr;
    }

    const std::string objectId = id.empty() ? "object-" + std::to_string(ObjectFactory::getNextRunningId()) : id;

    auto newPhysicsObject = std::make_unique<PhysicsObject>(objectId, SPHERE);

    // TODO: implement

    return GlobalPhysicsObjectStore::INSTANCE()->registerObject<PhysicsObject>(newPhysicsObject);
}

PhysicsObject * ObjectFactory::loadBox(const std::string id)
{
    if (!id.empty() && GlobalPhysicsObjectStore::INSTANCE()->getObjectById<PhysicsObject>(id) != nullptr) {
        logError("Object with id '" + id + "' already exists!");
        return nullptr;
    }

    const std::string objectId = id.empty() ? "object-" + std::to_string(ObjectFactory::getNextRunningId()) : id;

    auto newPhysicsObject = std::make_unique<PhysicsObject>(objectId, BOX);

    // TODO: implement

    return GlobalPhysicsObjectStore::INSTANCE()->registerObject<PhysicsObject>(newPhysicsObject);
}



const uint64_t ObjectFactory::getNextRunningId()
{
    std::lock_guard<std::mutex> lock(ObjectFactory::numberIncrementMutex);

    uint32_t ret = ObjectFactory::runningId;
    ObjectFactory::runningId++;

    return ret;
}

std::mutex ObjectFactory::numberIncrementMutex;
uint64_t ObjectFactory::runningId  = 0;

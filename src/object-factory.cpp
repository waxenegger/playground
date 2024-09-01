#include "includes/server.h"

PhysicsObject * ObjectFactory::loadModel(const std::string & modelFileLocation, const std::string & id, const unsigned int importerFlags, const bool useFirstChildAsRoot)
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

    float maxDistanceSquared = 0;
    const auto originalBBox = physicsObject->getOriginalBoundingBox();
    auto centerBBox = (originalBBox.max + originalBBox.min) / 2.0f;
    for (auto & v : m.vertices) {
        float distanceSquared = glm::distance2(v.position, centerBBox);
        maxDistanceSquared = std::max(distanceSquared, maxDistanceSquared);
    }
    physicsObject->setOriginalBoundingSphere({centerBBox, glm::sqrt(maxDistanceSquared)});

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

PhysicsObject * ObjectFactory::loadSphere(const std::string & id, const float radius)
{
    const std::string objectId = id.empty() ? "object-" + std::to_string(ObjectFactory::getNextRunningId()) : id;

    auto newPhysicsObject = std::make_unique<PhysicsObject>(objectId, SPHERE);

    BoundingSphere sphere = {
        {0.0f, 0.0f, 0.0f},
        radius
    };
    newPhysicsObject->setOriginalBoundingSphere(sphere);

    BoundingBox bbox;
    bbox.min = glm::vec3 { -radius };
    bbox.max = glm::vec3 { radius };
    newPhysicsObject->setOriginalBoundingBox(bbox);

    return GlobalPhysicsObjectStore::INSTANCE()->registerObject<PhysicsObject>(newPhysicsObject);
}

PhysicsObject * ObjectFactory::loadBox(const std::string & id, const float & width, const float & height, const float & depth)
{
    const std::string objectId = id.empty() ? "object-" + std::to_string(ObjectFactory::getNextRunningId()) : id;

    auto newPhysicsObject = std::make_unique<PhysicsObject>(objectId, BOX);

    const auto & middle = glm::vec3 {width, height, depth} * .5f;
    const float len = glm::sqrt(middle.x * middle.x + middle.y * middle.y + middle.z * middle.z);

    Mesh mesh;

    auto v = Vertex {{ middle.x, middle.y, middle.z  }, glm::vec3 { middle.x, middle.y, middle.z  } / len };
    newPhysicsObject->updateBboxWithVertex(v);
    mesh.vertices.emplace_back(v);

    v = Vertex { { middle.x, -middle.y, middle.z }, glm::vec3 { middle.x, -middle.y, middle.z  } / len };
    newPhysicsObject->updateBboxWithVertex(v);
    mesh.vertices.emplace_back(v);

    v = Vertex { { middle.x, -middle.y, -middle.z }, glm::vec3 { middle.x,-middle.y, -middle.z  } / len };
    newPhysicsObject->updateBboxWithVertex(v);
    mesh.vertices.emplace_back(v);

    v = Vertex { { middle.x, middle.y, -middle.z  }, glm::vec3 { middle.x, middle.y, -middle.z  } / len };
    newPhysicsObject->updateBboxWithVertex(v);
    mesh.vertices.emplace_back(v);

    v = Vertex { { -middle.x, -middle.y, -middle.z  }, glm::vec3 { -middle.x, -middle.y, -middle.z  } / len };
    newPhysicsObject->updateBboxWithVertex(v);
    mesh.vertices.emplace_back(v);

    v = Vertex { { -middle.x, -middle.y, middle.z  }, glm::vec3 { -middle.x, -middle.y, middle.z  } / len };
    newPhysicsObject->updateBboxWithVertex(v);
    mesh.vertices.emplace_back(v);

    v = Vertex { { -middle.x, middle.y, middle.z  }, glm::vec3 { -middle.x, middle.y, middle.z  } / len };
    newPhysicsObject->updateBboxWithVertex(v);
    mesh.vertices.emplace_back(v);

    v = Vertex { { -middle.x, middle.y, -middle.z  }, glm::vec3 { -middle.x, middle.y, -middle.z  } / len };
    newPhysicsObject->updateBboxWithVertex(v);
    mesh.vertices.emplace_back(v);

    const auto bbox = newPhysicsObject->getOriginalBoundingBox();
    newPhysicsObject->setOriginalBoundingSphere(bbox.getBoundingSphere());

    newPhysicsObject->addMesh(mesh);

    return GlobalPhysicsObjectStore::INSTANCE()->registerObject<PhysicsObject>(newPhysicsObject);
}

const uint64_t ObjectFactory::getNextRunningId()
{
    std::lock_guard<std::mutex> lock(ObjectFactory::numberIncrementMutex);

    uint32_t ret = ObjectFactory::runningId;
    ObjectFactory::runningId++;

    return ret;
}

PhysicsObject * ObjectFactory::handleCreateObjectRequest(const ObjectCreateRequest * request)
{
    const auto id = request->properties()->id()->str();
    auto existingObject = GlobalPhysicsObjectStore::INSTANCE()->getObjectById<PhysicsObject>(id);
    if (existingObject != nullptr) return existingObject;

    switch(request->object_type()) {
        case ObjectCreateRequestUnion_SphereCreateRequest:
        {
            const auto sphere = request->object_as_SphereCreateRequest();
            const auto radius = sphere->radius();

            auto sphereObject = ObjectFactory::loadSphere(id, radius);
            if (sphereObject == nullptr) return nullptr;
            sphereObject->setProperty<float>("radius", radius);
            sphereObject->setProperty<std::string>("texture", sphere->texture()->str());
            sphereObject->setProperty<Vec4>("color", *sphere->color());
            sphereObject->initProperties(request->properties()->location(), request->properties()->rotation(), request->properties()->scale());

            return sphereObject;
        }
        case ObjectCreateRequestUnion_BoxCreateRequest:
        {
            const auto box = request->object_as_BoxCreateRequest();
            const auto width = box->width();
            const auto height = box->height();
            const auto depth = box->depth();

            auto boxObject = ObjectFactory::loadBox(id, width, height, depth);
            if (boxObject == nullptr) return nullptr;

            boxObject->setProperty<float>("width", width);
            boxObject->setProperty<float>("height", height);
            boxObject->setProperty<float>("depth", depth);
            boxObject->setProperty<std::string>("texture", box->texture()->str());
            boxObject->setProperty<Vec4>("color", *box->color());
            boxObject->initProperties(request->properties()->location(), request->properties()->rotation(), request->properties()->scale());

            return boxObject;
        }
        case ObjectCreateRequestUnion_ModelCreateRequest:
        {
            const auto model = request->object_as_ModelCreateRequest();
            const auto file = (ObjectFactory::getAppPath(MODELS) / model->file()->str()).string();

            const auto flags =  model->flags();
            const auto useFirstChildAsRoot = model->first_child_root();

            auto modelObject = ObjectFactory::loadModel(file, id, flags, useFirstChildAsRoot);
            if (modelObject == nullptr) return nullptr;

            modelObject->setProperty<std::string>("file", model->file()->str());
            modelObject->setProperty<uint32_t>("flags", flags);
            modelObject->setProperty<bool>("useFirstChildAsRoot", useFirstChildAsRoot);

            modelObject->initProperties(request->properties()->location(), request->properties()->rotation(), request->properties()->scale());

            return modelObject;
        }
        case ObjectCreateRequestUnion_NONE:
            break;
    }

    return nullptr;
}

PhysicsObject * ObjectFactory::handleObjectPropertiesUpdateRequest(const ObjectPropertiesUpdateRequest * request)
{
    const auto id = request->id()->str();
    auto existingObject = GlobalPhysicsObjectStore::INSTANCE()->getObjectById<PhysicsObject>(id);
    if (existingObject == nullptr) return nullptr;

    const auto position = request->position();
    const auto rotation = request->rotation();
    const auto scaling = request->scaling();

    existingObject->setPosition({ position->x(), position->y(), position->z()});
    existingObject->setRotation({ rotation->x(), rotation->y(), rotation->z()});
    existingObject->setScaling(scaling);

    const auto animation = request->animation()->str();
    if (!animation.empty()) {
        existingObject->setCurrentAnimation(animation);
        existingObject->setCurrentAnimationTime(request->animation_time());
    }

    return existingObject;
}

bool ObjectFactory::handleCreateObjectResponse(CommBuilder & builder, const PhysicsObject * physicsObject)
{
    if (physicsObject == nullptr) return false;

    switch(physicsObject->getObjectType()) {
        case SPHERE:
        {
            const auto & matrix = physicsObject->getMatrix();
            const auto columns = std::array<Vec4,4> {
                Vec4 { matrix[0].x, matrix[1].x, matrix[2].x, matrix[3].x },
                Vec4 { matrix[0].y, matrix[1].y, matrix[2].y, matrix[3].y },
                Vec4 { matrix[0].z, matrix[1].z, matrix[2].z, matrix[3].z },
                Vec4 { matrix[0].w, matrix[1].w, matrix[2].w, matrix[3].w }
            };
            const auto sphere = physicsObject->getBoundingSphere();

            const auto rotation = Vec3 {
                physicsObject->getRotation().x,
                physicsObject->getRotation().y,
                physicsObject->getRotation().z,
            };

            CommCenter::addObjectCreateAndUpdateSphereRequest(
                builder,
                physicsObject->getId(),
                sphere.radius,
                Vec3 {sphere.center.x, sphere.center.y, sphere.center.z },
                columns,
                rotation,
                physicsObject->getScaling(),
                physicsObject->getProperty<float>("radius", 0.0f),
                physicsObject->getProperty<Vec4>("color", Vec4(1.0f,1.0f,1.0f,1.0f)),
                physicsObject->getProperty<std::string>("texture", "")
            );
            break;
        }
        case BOX:
        {
            const auto & matrix = physicsObject->getMatrix();
            const auto columns = std::array<Vec4,4> {
                Vec4 { matrix[0].x, matrix[1].x, matrix[2].x, matrix[3].x },
                Vec4 { matrix[0].y, matrix[1].y, matrix[2].y, matrix[3].y },
                Vec4 { matrix[0].z, matrix[1].z, matrix[2].z, matrix[3].z },
                Vec4 { matrix[0].w, matrix[1].w, matrix[2].w, matrix[3].w }
            };
            const auto sphere = physicsObject->getBoundingSphere();
            const auto rotation = Vec3 {
                physicsObject->getRotation().x,
                physicsObject->getRotation().y,
                physicsObject->getRotation().z,
            };

            CommCenter::addObjectCreateAndUpdateBoxRequest(
                builder,
                physicsObject->getId(),
                sphere.radius,
                Vec3 {sphere.center.x, sphere.center.y, sphere.center.z },
                columns,
                rotation,
                physicsObject->getScaling(),
                physicsObject->getProperty<float>("width", 0.0f),
                physicsObject->getProperty<float>("height", 0.0f),
                physicsObject->getProperty<float>("depth", 0.0f),
                physicsObject->getProperty<Vec4>("color", Vec4(1.0f,1.0f,1.0f,1.0f)),
                physicsObject->getProperty<std::string>("texture", "")
            );

            break;
        }
        case MODEL:
        {
            const auto & matrix = physicsObject->getMatrix();
            const auto columns = std::array<Vec4,4> {
                Vec4 { matrix[0].x, matrix[1].x, matrix[2].x, matrix[3].x },
                Vec4 { matrix[0].y, matrix[1].y, matrix[2].y, matrix[3].y },
                Vec4 { matrix[0].z, matrix[1].z, matrix[2].z, matrix[3].z },
                Vec4 { matrix[0].w, matrix[1].w, matrix[2].w, matrix[3].w }
            };
            const auto sphere = physicsObject->getBoundingSphere();
            const auto rotation = Vec3 {
                physicsObject->getRotation().x,
                physicsObject->getRotation().y,
                physicsObject->getRotation().z,
            };

            CommCenter::addObjectCreateAndUpdateModelRequest(
                builder,
                physicsObject->getId(),
                sphere.radius,
                Vec3 { sphere.center.x, sphere.center.y, sphere.center.z },
                columns,
                rotation,
                physicsObject->getScaling(),
                physicsObject->getProperty<std::string>("file", ""),
                physicsObject->getCurrentAnimation(),
                physicsObject->getCurrentAnimationTime(),
                physicsObject->getProperty<uint32_t>("flags", 0),
                physicsObject->getProperty<bool>("useFirstChildAsRoot", false)
            );

            break;
        }
        default:
            return false;
    }

    return true;
}

void ObjectFactory::addDebugResponse(CommBuilder & builder, const PhysicsObject * physicsObject)
{
    if (physicsObject == nullptr) return;

    const auto boundingSphere = physicsObject->getBoundingSphere();
    const auto boundingBox = physicsObject->getBoundingBox();

    CommCenter::addObjectDebugRequest(
        builder, physicsObject->getId() + "-debug",
        boundingSphere.radius,
        { boundingSphere.center.x, boundingSphere.center.y, boundingSphere.center.z },
        { boundingBox.min.x, boundingBox.min.y, boundingBox.min.z },
        { boundingBox.max.x, boundingBox.max.y, boundingBox.max.z }
    );
}

bool ObjectFactory::handleCreateUpdateResponse(CommBuilder & builder, const PhysicsObject * physicsObject)
{
    if (physicsObject == nullptr) return false;

    const auto & matrix = physicsObject->getMatrix();
    const auto columns = std::array<Vec4,4> {
        Vec4 { matrix[0].x, matrix[1].x, matrix[2].x, matrix[3].x },
        Vec4 { matrix[0].y, matrix[1].y, matrix[2].y, matrix[3].y },
        Vec4 { matrix[0].z, matrix[1].z, matrix[2].z, matrix[3].z },
        Vec4 { matrix[0].w, matrix[1].w, matrix[2].w, matrix[3].w }
    };
    const auto sphere = physicsObject->getBoundingSphere();
    const auto rotation = Vec3 {
        physicsObject->getRotation().x,physicsObject->getRotation().y, physicsObject->getRotation().z
    };

    CommCenter::addObjectUpdateRequest(
        builder,
        physicsObject->getId(),
        sphere.radius,
        Vec3 {sphere.center.x, sphere.center.y, sphere.center.z },
        columns,
        rotation,
        physicsObject->getScaling(),
        physicsObject->getCurrentAnimation(),
        physicsObject->getCurrentAnimationTime()
    );

    return true;
}

std::filesystem::path ObjectFactory::getAppPath(APP_PATHS appPath)
{
    return ::getAppPath(ObjectFactory::base, appPath);
}

 std::filesystem::path ObjectFactory::base = "";
std::mutex ObjectFactory::numberIncrementMutex;
uint64_t ObjectFactory::runningId  = 0;

#include "includes/physics.h"

PhysicsObject::PhysicsObject(const std::string id, ObjectType objectType) : id(id), type(objectType){}

PhysicsObject::~PhysicsObject() {}

void PhysicsObject::setDirty(const bool & dirty) {
    this->dirty = dirty;
}

bool PhysicsObject::isDirty() const {
    return this->dirty;
}

void PhysicsObject::setPosition(const glm::vec3 position) {
    if (position == this->position) return;

    const glm::mat4 oldMatrix = this->matrix;

    this->position = position;
    this->updateMatrix();

    //this->updateBbox(oldMatrix);
}

const std::set<std::string> PhysicsObject::getOrUpdateSpatialHashKeys(const bool updateHashKeys)
{
    const std::lock_guard<std::mutex> lock(this->spatialHashKeysMutex);

    if (!updateHashKeys) return this->spatialHashKeys;

    std::set<std::string> spatialKeys;

    const int minX = glm::floor(this->bbox.min.x);
    int maxX = glm::floor(this->bbox.max.x);
    if (maxX == minX) maxX+= UNIFORM_GRID_CELL_LENGTH;
    else maxX += UNIFORM_GRID_CELL_LENGTH - ((maxX-minX) % UNIFORM_GRID_CELL_LENGTH);

    const int minY = glm::floor(this->bbox.min.y);
    int maxY = glm::floor(this->bbox.max.y);
    if (maxY == minY) maxY+= UNIFORM_GRID_CELL_LENGTH;
    else maxY += UNIFORM_GRID_CELL_LENGTH - ((maxY-minY) % UNIFORM_GRID_CELL_LENGTH);

    const int minZ = glm::floor(this->bbox.min.z);
    int maxZ = glm::floor(this->bbox.max.z);
    if (maxZ == minZ) maxZ+= UNIFORM_GRID_CELL_LENGTH;
    else maxZ += UNIFORM_GRID_CELL_LENGTH - ((maxZ-minZ) % UNIFORM_GRID_CELL_LENGTH);

    for (int x=minX;x<=maxX;x+=UNIFORM_GRID_CELL_LENGTH)
        for (int y=minY;y<=maxY;y+=UNIFORM_GRID_CELL_LENGTH)
            for (int z=minZ;z<=maxZ;z+=UNIFORM_GRID_CELL_LENGTH) {
                int signX = glm::sign(x);
                int indX = x / UNIFORM_GRID_CELL_LENGTH;
                if (signX < 0) indX--;

                int signY = glm::sign(y);
                int indY = y / UNIFORM_GRID_CELL_LENGTH;
                if (signY < 0) indY--;

                int signZ = glm::sign(z);
                int indZ = z / UNIFORM_GRID_CELL_LENGTH;
                if (signZ < 0) indZ--;

                spatialKeys.emplace(std::to_string(indX) + "|" + std::to_string(indY) + "|" +  std::to_string(indZ));
            }

    if (!this->spatialHashKeys.empty() && !spatialKeys.empty()) SpatialHashMap::INSTANCE()->updateObject(this->spatialHashKeys, spatialKeys, this);

    this->spatialHashKeys = spatialKeys;

    return this->spatialHashKeys;
}

void PhysicsObject::setScaling(const float factor) {
    if (factor <= 0 || factor == this->scaling) return;

    const glm::mat4 oldMatrix = this->matrix;

    this->scaling = factor;
    this->updateMatrix();

    //this->updateBbox(oldMatrix);
}

void PhysicsObject::setRotation(glm::vec3 rotation) {
    if (rotation == this->rotation) return;

    const glm::mat4 oldMatrix = this->matrix;

    this->rotation = rotation;
    this->updateMatrix();

    //this->updateBbox(oldMatrix, true);
}

const glm::vec3 PhysicsObject::getPosition() const {
    return this->position;
}

const glm::vec3 PhysicsObject::getRotation() const {
    return this->rotation;
}

const float PhysicsObject::getScaling() const {
    return this->scaling;
}

const glm::mat4 PhysicsObject::getMatrix() const
{
    return this->matrix;
}

const std::string PhysicsObject::getId() const
{
    return this->id;
}

void PhysicsObject::updateMatrix() {
    glm::mat4 transformation = glm::mat4(1.0f);

    transformation = glm::translate(transformation, this->position);

    if (this->rotation.x != 0.0f) transformation = glm::rotate(transformation, this->rotation.x, glm::vec3(1, 0, 0));
    if (this->rotation.y != 0.0f) transformation = glm::rotate(transformation, this->rotation.y, glm::vec3(0, 1, 0));
    if (this->rotation.z != 0.0f) transformation = glm::rotate(transformation, this->rotation.z, glm::vec3(0, 0, 1));

    this->matrix = glm::scale(transformation, glm::vec3(this->scaling));

    this->dirty = true;
}

std::vector<Mesh> & PhysicsObject::getMeshes()
{
    return this->meshes;
}

void PhysicsObject::updateBboxWithVertex(const Vertex & vertex)
{
    this->bbox.min.x = std::min(vertex.position.x, this->bbox.min.x);
    this->bbox.min.y = std::min(vertex.position.y, this->bbox.min.y);
    this->bbox.min.z = std::min(vertex.position.z, this->bbox.min.z);

    this->bbox.max.x = std::max(vertex.position.x, this->bbox.max.x);
    this->bbox.max.y = std::max(vertex.position.y, this->bbox.max.y);
    this->bbox.max.z = std::max(vertex.position.z, this->bbox.max.z);
}

void PhysicsObject::addMesh(const Mesh & mesh)
{
    this->meshes.emplace_back(std::move(mesh));
}

void PhysicsObject::addVertexJointInfo(const VertexJointInfo & vertexJointInfo)
{
    this->vertexJointInfo.emplace_back(std::move(vertexJointInfo));
}

const std::optional<uint32_t> PhysicsObject::getJointIndexByName(const std::string & name)
{
    if (!this->jointIndexByName.contains(name)) return std::nullopt;

    return this->jointIndexByName[name];
}

void PhysicsObject::updateJointIndexByName(const std::string & name, std::optional<uint32_t> value)
{
    this->jointIndexByName[name] = value.has_value() ? value.value() : this->jointIndexByName[name] = this->jointIndexByName.size() ;
}

void PhysicsObject::addJointInformation(const JointInformation & jointInfo)
{
    this->joints.emplace_back(std::move(jointInfo));
}

void PhysicsObject::updateVertexJointInfo(const uint32_t offset, const uint32_t jointIndex, float jointWeight)
{
    if (offset >= this->vertexJointInfo.size() || jointWeight <= 0.0f) return;

    VertexJointInfo & jointInfo = this->vertexJointInfo[offset];

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

void PhysicsObject::reserveJoints()
{
    this->joints.reserve(MAX_JOINTS);
}

void PhysicsObject::populateJoints(const aiScene * scene, const aiNode * root)
{
    if (this->jointIndexByName.empty()) return;

    this->joints.resize(this->jointIndexByName.size());

    const aiMatrix4x4 & trans = root->mTransformation;
    glm::mat4 rootNodeTransform = {
        { trans.a1, trans.b1, trans.c1, trans.d1 },
        { trans.a2, trans.b2, trans.c2, trans.d2 },
        { trans.a3, trans.b3, trans.c3, trans.d3 },
        { trans.a4, trans.b4, trans.c4, trans.d4 }
    };

    NodeInformation rootNode = { };
    rootNode.name = std::string(root->mName.C_Str(), root->mName.length);
    rootNode.transformation = rootNodeTransform;

    this->rootNode = rootNode;
    this->rootInverseTransformation = glm::inverse(rootNodeTransform);

    this->processJoints(root, this->rootNode, -1, true);
    this->processAnimations(scene);
}

void PhysicsObject::processJoints(const aiNode * node, NodeInformation & parentNode, int32_t parentIndex, bool isRoot)
{
    int32_t childIndex = -1;

    const std::string nodeName = std::string(node->mName.C_Str(), node->mName.length);

    const aiMatrix4x4 & trans = node->mTransformation;
    const glm::mat4 nodeTransformation = glm::mat4 {
        { trans.a1, trans.b1, trans.c1, trans.d1 },
        { trans.a2, trans.b2, trans.c2, trans.d2 },
        { trans.a3, trans.b3, trans.c3, trans.d3 },
        { trans.a4, trans.b4, trans.c4, trans.d4 }
    };

    if (!nodeName.empty() && this->jointIndexByName.contains(nodeName)) {

        childIndex = this->jointIndexByName[nodeName];

        JointInformation & joint = this->joints[childIndex];
        joint.nodeTransformation = nodeTransformation;

        if (parentIndex != -1) {
            JointInformation & parentJoint = this->joints[parentIndex];
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
        this->processJoints(node->mChildren[i], isRoot ? parentNode : parentNode.children[parentNode.children.size()-1], childIndex);
    }
}

void PhysicsObject::processAnimations(const aiScene *scene)
{
    for(unsigned int i=0; i < scene->mNumAnimations; i++) {
        const aiAnimation * anim = scene->mAnimations[i];

        std::string animName = "anim" + std::to_string(this->animations.size());
        if (anim->mName.length > 0) {
            animName = std::string(anim->mName.C_Str(), anim->mName.length);
        }

        if (i==0) this->currentAnimation = animName;

        AnimationInformation animInfo {};
        animInfo.duration = anim->mDuration;
        animInfo.ticksPerSecond = anim->mTicksPerSecond;

        for(unsigned int j=0; j < anim->mNumChannels; j++) {
            const aiNodeAnim * details = anim->mChannels[j];

            AnimationDetails animDetails {};
            animDetails.name = std::string(details->mNodeName.C_Str(), details->mNodeName.length);

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

        this->animations[animName] = animInfo;
    }
}

void PhysicsObject::flagAsRegistered() {
    this->registered = true;
}

bool PhysicsObject::hasBeenRegistered() {
    return this->registered;
}

/*
void PhysicsObject::updateBbox(const glm::mat4 & oldMatrix, const bool forceRecalculation) {
    if (oldMatrix == glm::mat4(0)) return;

    if (forceRecalculation) {
        this->recalculateBoundingBox();
        this->getOrUpdateSpatialHashKeys(true);
        return;
    }

    const glm::mat4 invMatrix = glm::inverse(oldMatrix);
    glm::vec4 prevMin = invMatrix * glm::vec4(this->bbox.min, 1.0);
    glm::vec4 prevMax = invMatrix * glm::vec4(this->bbox.max, 1.0);

    glm::vec3 minTransformed =  this->matrix * prevMin;
    glm::vec3 maxTransformed =  this->matrix * prevMax;

    this->bbox = Helper::createBoundingBoxFromMinMax(minTransformed, maxTransformed);

    this->getOrUpdateSpatialHashKeys(true);
}
*/

void PhysicsObject::rotate(int xAxis, int yAxis, int zAxis) {
    glm::vec3 rot {
        glm::radians(static_cast<float>(xAxis)),
        glm::radians(static_cast<float>(yAxis)),
        glm::radians(static_cast<float>(zAxis))
    };

    rot = this->rotation + rot;
    this->setRotation(rot);
}

void PhysicsObject::move(const float delta, const Direction & direction) {
    if (delta == 0.0f) return;

    glm::vec3 deltaPosition = this->position;

    if (direction.up) {
        deltaPosition += this->getUnitDirectionVector() * delta;
    } else if (direction.left) {
        deltaPosition += this->getUnitDirectionVector(PI_HALF) * delta;
    } else if (direction.right) {
        deltaPosition += this->getUnitDirectionVector(-PI_HALF) * delta;
    } else if (direction.down) {
        deltaPosition -= this->getUnitDirectionVector() * delta;
    }

    this->setPosition(deltaPosition);
}

glm::vec3 PhysicsObject::getUnitDirectionVector(const float leftRightAngle) {
    glm::vec3 frontOfComponent(
        cos(this->rotation.x) * sin(this->rotation.y+leftRightAngle),
        sin(this->rotation.x),
        cos(this->rotation.x) * cos(this->rotation.y+leftRightAngle));
    frontOfComponent = glm::normalize(frontOfComponent);

    return frontOfComponent;
}

SpatialHashMap::SpatialHashMap() {}

SpatialHashMap * SpatialHashMap::INSTANCE()
{
    if (SpatialHashMap::instance == nullptr) {
        SpatialHashMap::instance = new SpatialHashMap();
    }

    return SpatialHashMap::instance;
}

void SpatialHashMap::updateObject(std::set<std::string> oldIndices, std::set<std::string> newIndices, PhysicsObject * physicsObject)
{
    std::vector<std::string> indToBeRemoved;
    std::vector<std::string> indToBeAdded;

    for (auto & i : oldIndices) {
        if (!newIndices.contains(i)) indToBeRemoved.push_back(i);
    }

    for (auto & i : newIndices) {
        if (!oldIndices.contains(i)) indToBeAdded.push_back(i);
    }

    for (auto & i : indToBeAdded) {
        const auto hit = this->gridMap.find(i);
        if (hit != this->gridMap.end()) hit->second.emplace_back(physicsObject);
        else this->gridMap[i] = { physicsObject };
    }

    for (auto & i : indToBeRemoved) {
        const auto hit = this->gridMap.find(i);
        if (hit != this->gridMap.end()) hit->second.erase(std::remove(hit->second.begin(), hit->second.end(), physicsObject), hit->second.end());
    }
}

void SpatialHashMap::addObject(PhysicsObject * physicsObject)
{
    if (physicsObject == nullptr) return;

    auto keys = physicsObject->getOrUpdateSpatialHashKeys(true);
    for (auto & k : keys) {
        const auto existingKey = this->gridMap.find(k);
        if (existingKey != this->gridMap.end()) {
            existingKey->second.emplace_back(physicsObject);
        } else {
            this->gridMap[k] = { physicsObject };
        }
    }
}

ankerl::unordered_dense::map<std::string, std::set<PhysicsObject *>> SpatialHashMap::performBroadPhaseCollisionCheck(const std::vector<PhysicsObject *> & physicsObjects)
{
    ankerl::unordered_dense::map<std::string, std::set<PhysicsObject *>> collisions;

    for (auto r : physicsObjects) {
        const std::set<std::string> & spatialIndices = r->getOrUpdateSpatialHashKeys();
        // iterate over all (partially) occupied space
        for (auto & i: spatialIndices) {

            // for each cell find the list of other objects in it
            // exit early if nothing is there or only our own entry
            auto it = this->gridMap.find(i);
            if (it == this->gridMap.end() || it->second.size() <= 1) continue;

            const auto & objectsInGridCell = it->second;
            for (auto j : objectsInGridCell) {
                // no self checks
                if (j == r) continue;

                // check if we have already added the collision
                //const auto itCollisionsR = collisions.find(r->getName());
                //if (itCollisionsR != collisions.end() && itCollisionsR->second.contains(j)) continue;

                // check if we had a collision already (from opposite order check)
                const auto itCollisions = collisions.find(j->getId());
                if (itCollisions != collisions.end() && itCollisions->second.contains(r)) continue;

                // TODO: implement
                /*
                if (Helper::checkBBoxIntersection(r->getBoundingBox(), j->getBoundingBox())) {
                    collisions[r->getName()].emplace(j);
                }
                */
            }
       }
    }

    return collisions;
}

void PhysicsObject::recalculateBoundingBox()
{
    BoundingBox newBoundingBox;

    const bool hasAnimations = !this->animationMatrices.empty();

    uint32_t i=0;
    for (const auto & m : this->meshes) {
        for (const auto & v : m.vertices) {
            glm::vec4 transformedPosition = glm::vec4(v.position, 1.0f);

            if (hasAnimations) {
                const glm::mat4 animationMatrix = i >= this->animationMatrices.size() ? glm::mat4 { 1.0f } : this->animationMatrices[i];
                glm::vec4 tmpVec = animationMatrix * transformedPosition;
                transformedPosition = this->matrix * (tmpVec / tmpVec.w);
            } else {
                transformedPosition = this->matrix * transformedPosition;
            }

            newBoundingBox.min.x = glm::min(newBoundingBox.min.x, transformedPosition.x);
            newBoundingBox.min.y = glm::min(newBoundingBox.min.y, transformedPosition.y);
            newBoundingBox.min.z = glm::min(newBoundingBox.min.z, transformedPosition.z);

            newBoundingBox.max.x = glm::max(newBoundingBox.max.x, transformedPosition.x);
            newBoundingBox.max.y = glm::max(newBoundingBox.max.y, transformedPosition.y);
            newBoundingBox.max.z = glm::max(newBoundingBox.max.z, transformedPosition.z);
            i++;
        }
    }

    this->bbox = newBoundingBox;
};

SpatialHashMap::~SpatialHashMap() {
    if (SpatialHashMap::instance == nullptr) return;

    this->gridMap.clear();

    delete SpatialHashMap::instance;

    SpatialHashMap::instance = nullptr;
}

SpatialHashMap * SpatialHashMap::instance = nullptr;

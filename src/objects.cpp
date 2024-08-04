#include "includes/helper.h"

Renderable::Renderable(const std::string name) : name(name){}

Renderable::~Renderable() {}

void Renderable::setDirty(const bool & dirty) {
    this->dirty = dirty;
}

bool Renderable::isDirty() const {
    return this->dirty;
}

void Renderable::flagAsRegistered() {
    this->registered = true;
}

bool Renderable::hasBeenRegistered() {
    return this->registered;
}

bool Renderable::shouldBeRendered() const
{
    return this->frustumCulled;
}

void Renderable::setPosition(const glm::vec3 position) {
    if (position == this->position) return;

    const glm::mat4 oldMatrix = this->matrix;

    this->position = position;
    this->updateMatrix();

    this->updateBbox(oldMatrix);

    for (auto & r : this->debugRenderable) {
        r->setPosition(this->position);
    }
}

const std::set<std::string> Renderable::getOrUpdateSpatialHashKeys(const bool updateHashKeys)
{
    const std::lock_guard<std::mutex> lock(this->spatialHashKeysMutex);

    if (!updateHashKeys || !this->hasBeenRegistered()) return this->spatialHashKeys;

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

    if (!this->spatialHashKeys.empty() && !spatialKeys.empty()) SpatialRenderableStore::INSTANCE()->updateRenderable(this->spatialHashKeys, spatialKeys, this);

    this->spatialHashKeys = spatialKeys;

    return this->spatialHashKeys;
}

const BoundingBox Renderable::getBoundingBox(const bool& withoutTransformations) const
{
    if (!withoutTransformations) return this->bbox;

    const auto & inverseMatrix = glm::inverse(this->matrix);

    glm::vec3 minTransformed =  inverseMatrix * glm::vec4(this->bbox.min, 1.0f);
    glm::vec3 maxTransformed =  inverseMatrix * glm::vec4(this->bbox.max, 1.0f);

    return Helper::createBoundingBoxFromMinMax(minTransformed, maxTransformed);
}

void Renderable::setScaling(const float factor) {
    if (factor <= 0 || factor == this->scaling) return;

    const glm::mat4 oldMatrix = this->matrix;

    this->scaling = factor;
    this->updateMatrix();

    this->updateBbox(oldMatrix);

    for (auto & r : this->debugRenderable) {
        r->setScaling(this->scaling);
    }
}

void Renderable::setRotation(glm::vec3 rotation) {
    if (rotation == this->rotation) return;

    const glm::mat4 oldMatrix = this->matrix;

    this->rotation = rotation;
    this->updateMatrix();

    this->updateBbox(oldMatrix, true);

    for (auto & r : this->debugRenderable) {
        r->setRotation(this->rotation);
    }
}

const glm::vec3 Renderable::getPosition() const {
    return this->position;
}

const glm::vec3 Renderable::getRotation() const {
    return this->rotation;
}

const float Renderable::getScaling() const {
    return this->scaling;
}

const glm::mat4 Renderable::getMatrix() const
{
    return this->matrix;
}

void Renderable::addDebugRenderable(Renderable * renderable)
{
    renderable->setPosition(this->getPosition());
    renderable->setRotation(this->getRotation());
    renderable->setScaling(this->getScaling());

    this->debugRenderable.emplace_back(renderable);
}

void Renderable::performFrustumCulling(const std::array<glm::vec4, 6> & frustumPlanes) {
    if (this->bbox.min[0] == INF && this->bbox.max[0] == NEG_INF) {
        this->frustumCulled = true;
        return;
    }

    for (int i = 0; i < 6; i++) {
        if (glm::dot(glm::vec4(this->bbox.center, 1), frustumPlanes[i]) + this->bbox.radius < 0.0f) {
            this->frustumCulled = false;
            return;
        }
    }

    this->frustumCulled = true;
}

const std::string Renderable::getName() const
{
    return this->name;
}

void Renderable::updateMatrix() {
    glm::mat4 transformation = glm::mat4(1.0f);

    transformation = glm::translate(transformation, this->position);

    if (this->rotation.x != 0.0f) transformation = glm::rotate(transformation, this->rotation.x, glm::vec3(1, 0, 0));
    if (this->rotation.y != 0.0f) transformation = glm::rotate(transformation, this->rotation.y, glm::vec3(0, 1, 0));
    if (this->rotation.z != 0.0f) transformation = glm::rotate(transformation, this->rotation.z, glm::vec3(0, 0, 1));

    this->matrix = glm::scale(transformation, glm::vec3(this->scaling));

    this->dirty = true;
}

void Renderable::updateBbox(const glm::mat4 & oldMatrix, const bool forceRecalculation) {
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

void Renderable::rotate(int xAxis, int yAxis, int zAxis) {
    glm::vec3 rot {
        glm::radians(static_cast<float>(xAxis)),
        glm::radians(static_cast<float>(yAxis)),
        glm::radians(static_cast<float>(zAxis))
    };

    rot = this->rotation + rot;
    this->setRotation(rot);
}

void Renderable::move(const float delta, const Direction & direction) {
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

glm::vec3 Renderable::getUnitDirectionVector(const float leftRightAngle) {
    glm::vec3 frontOfComponent(
        cos(this->rotation.x) * sin(this->rotation.y+leftRightAngle),
        sin(this->rotation.x),
        cos(this->rotation.x) * cos(this->rotation.y+leftRightAngle));
    frontOfComponent = glm::normalize(frontOfComponent);

    return frontOfComponent;
}

GlobalRenderableStore::GlobalRenderableStore() {}

GlobalRenderableStore * GlobalRenderableStore::INSTANCE()
{
    if (GlobalRenderableStore::instance == nullptr) {
        GlobalRenderableStore::instance = new GlobalRenderableStore();
    }

    return GlobalRenderableStore::instance;
}

GlobalRenderableStore::~GlobalRenderableStore() {
    if (GlobalRenderableStore::instance == nullptr) return;

    this->objects.clear();

    delete GlobalRenderableStore::instance;

    GlobalRenderableStore::instance = nullptr;
}

void GlobalRenderableStore::performFrustumCulling(const std::array<glm::vec4, 6> & frustumPlanes)
{
    const std::lock_guard<std::mutex> lock(this->registrationMutex);

    std::for_each(
        std::execution::par,
        this->objects.begin(),
        this->objects.end(),
        [&frustumPlanes](auto & renderable) {
            renderable->performFrustumCulling(frustumPlanes);
        }
    );
}

uint32_t GlobalRenderableStore::getNumberOfRenderables()
{
    return this->objects.size();
}

GlobalRenderableStore * GlobalRenderableStore::instance = nullptr;

SpatialRenderableStore::SpatialRenderableStore() {}

SpatialRenderableStore * SpatialRenderableStore::INSTANCE()
{
    if (SpatialRenderableStore::instance == nullptr) {
        SpatialRenderableStore::instance = new SpatialRenderableStore();
    }

    return SpatialRenderableStore::instance;
}

void SpatialRenderableStore::updateRenderable(std::set<std::string> oldIndices, std::set<std::string> newIndices, Renderable * renderable)
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
        if (hit != this->gridMap.end()) hit->second.emplace_back(renderable);
        else this->gridMap[i] = { renderable };
    }

    for (auto & i : indToBeRemoved) {
        const auto hit = this->gridMap.find(i);
        if (hit != this->gridMap.end()) hit->second.erase(std::remove(hit->second.begin(), hit->second.end(), renderable), hit->second.end());
    }
}

void SpatialRenderableStore::addRenderable(Renderable * renderable)
{
    if (renderable == nullptr) return;

    auto keys = renderable->getOrUpdateSpatialHashKeys(true);
    for (auto & k : keys) {
        const auto existingKey = this->gridMap.find(k);
        if (existingKey != this->gridMap.end()) {
            existingKey->second.emplace_back(renderable);
        } else {
            this->gridMap[k] = { renderable };
        }
    }
}

ankerl::unordered_dense::map<std::string, std::set<Renderable *>> SpatialRenderableStore::performBroadPhaseCollisionCheck(const std::vector<Renderable *> & renderables)
//std::unordered_map<std::string, std::set<Renderable *>> SpatialRenderableStore::performBroadPhaseCollisionCheck(const std::vector<Renderable *> & renderables)
{
    ankerl::unordered_dense::map<std::string, std::set<Renderable *>> collisions;
    //std::unordered_map<std::string, std::set<Renderable *>> collisions;

    for (auto r : renderables) {
        const std::set<std::string> & spatialIndices = r->getOrUpdateSpatialHashKeys();
        // iterate over all (partially) occupied space
        for (auto & i: spatialIndices) {

            // for each cell find the list of other objects in it
            // exit early if nothing is there or only our own entry
            auto it = this->gridMap.find(i);
            if (it == this->gridMap.end() || it->second.size() <= 1) continue;

            const auto & renderablesInGridCell = it->second;
            std::vector<Renderable *> renderablesToCheck;
            for (auto j : renderablesInGridCell) {
                // no self checks
                if (j == r) continue;

                // check if we have already added the collision
                //const auto itCollisionsR = collisions.find(r->getName());
                //if (itCollisionsR != collisions.end() && itCollisionsR->second.contains(j)) continue;

                // check if we had a collision already (from opposite order check)
                const auto itCollisions = collisions.find(j->getName());
                if (itCollisions != collisions.end() && itCollisions->second.contains(r)) continue;

                if (Helper::checkBBoxIntersection(r->getBoundingBox(), j->getBoundingBox())) {
                    collisions[r->getName()].emplace(j);
                }
            }
       }
    }

    return collisions;
}

SpatialRenderableStore::~SpatialRenderableStore() {
    if (SpatialRenderableStore::instance == nullptr) return;

    this->gridMap.clear();

    delete SpatialRenderableStore::instance;

    SpatialRenderableStore::instance = nullptr;
}

SpatialRenderableStore * SpatialRenderableStore::instance = nullptr;

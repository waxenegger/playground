#include "includes/objects.h"

Renderable::Renderable() {}

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

bool Renderable::shouldBeRendered(const std::array<glm::vec4, 6> & frustumPlanes) const
{
    return this->isInFrustum(frustumPlanes);
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

    this->updateBbox(oldMatrix);

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

bool Renderable::isInFrustum(const std::array<glm::vec4, 6> & frustumPlanes) const {
    if (this->bbox.min[0] == INF && this->bbox.max[0] == NEG_INF) return true;

    for (int i = 0; i < 6; i++) {
        if (glm::dot(glm::vec4(this->bbox.center, 1), frustumPlanes[i]) + this->bbox.radius < 0.0f) {
            return false;
        }
    }

    return true;
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

void Renderable::updateBbox(const glm::mat4 & oldMatrix) {
    if (oldMatrix == glm::mat4(0)) return;

    const glm::mat4 invMatrix = glm::inverse(oldMatrix);
    glm::vec4 prevMin = invMatrix * glm::vec4(this->bbox.min, 1.0);
    glm::vec4 prevMax = invMatrix * glm::vec4(this->bbox.max, 1.0);

    glm::vec3 minTransformed =  this->matrix * prevMin;
    glm::vec3 maxTransformed =  this->matrix * prevMax;

    this->bbox = Helper::createBoundingBoxFromMinMax(minTransformed, maxTransformed);
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
        deltaPosition += this->getFront() * delta;
    } else if (direction.left) {
        deltaPosition += this->getFront(PI_HALF) * delta;
    } else if (direction.right) {
        deltaPosition += this->getFront(-PI_HALF) * delta;
    } else if (direction.down) {
        deltaPosition -= this->getFront() * delta;
    }

    this->setPosition(deltaPosition);
}

glm::vec3 Renderable::getFront(const float leftRightAngle) {
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


GlobalRenderableStore * GlobalRenderableStore::instance = nullptr;


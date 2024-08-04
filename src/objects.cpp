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
    return !this->frustumCulled;
}

void Renderable::setPosition(const glm::vec3 position) {
    if (position == this->position) return;

    this->position = position;
    this->updateMatrix();
}

const BoundingBox Renderable::getBoundingBox() const
{
    return this->bbox;
}

void Renderable::setBoundingBox(const BoundingBox bbox)
{
    this->bbox = bbox;
    this->sphere = this->bbox.getBoundingSphere();
}

const BoundingSphere Renderable::getBoundingSphere() const
{
    return this->sphere;
}

void Renderable::setScaling(const float factor) {
    if (factor <= 0 || factor == this->scaling) return;

    this->scaling = factor;
    this->updateMatrix();
}

void Renderable::setRotation(glm::vec3 rotation) {
    if (rotation == this->rotation) return;

    this->rotation = rotation;
    this->updateMatrix();
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

void Renderable::performFrustumCulling(const std::array<glm::vec4, 6> & frustumPlanes) {
    if (this->sphere.radius == 0.0f) {
        this->frustumCulled = false;
        return;
    }

    for (int i = 0; i < 6; i++) {
        if (glm::dot(glm::vec4(this->sphere.center, 1), frustumPlanes[i]) + this->sphere.radius < 0.0f) {
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

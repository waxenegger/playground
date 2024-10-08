#include "includes/helper.h"

Renderable::Renderable(const std::string id) : id(id){}

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

const BoundingSphere Renderable::getBoundingSphere() const
{
    return this->sphere;
}

void Renderable::setBoundingSphere(const BoundingSphere & sphere) {
    this->sphere = sphere;
}

const glm::mat4 Renderable::getMatrix() const
{
    return this->matrix;
}

void Renderable::setRotation(const glm::vec3 rotation)
{
    this->rotation = rotation;
}

const glm::vec3 Renderable::getRotation() const
{
    return this->rotation;
}

const glm::vec3 Renderable::getPosition() const
{
    return this->position;
}

void Renderable::setPosition(const glm::vec3 position)
{
    this->position = position;
}

void Renderable::setScaling(const float scaling)
{
    this->scaling = scaling;
}

float Renderable::getScaling() const
{
    return this->scaling;
}

bool Renderable::hasAnimation() const
{
    return this->isAnimatedModel;
}

void Renderable::performFrustumCulling(const std::array<glm::vec4, 6> & frustumPlanes) {
    if (this->sphere.radius == 0.0f) {
        this->frustumCulled = true;
        return;
    }

    for (int i = 0; i < 6; i++) {
        if (glm::dot(glm::vec4(this->sphere.center, 1), frustumPlanes[i]) + this->sphere.radius < 0.0f) {
            this->frustumCulled = true;
            return;
        }
    }

    this->frustumCulled = false;
}

const std::string Renderable::getId() const
{
    return this->id;
}

void Renderable::setMatrix(const Matrix * matrix) {
    this->matrix =
    {
        { matrix->col0()->x(), matrix->col1()->x(), matrix->col2()->x(), matrix->col3()->x() },
        { matrix->col0()->y(), matrix->col1()->y(), matrix->col2()->y(), matrix->col3()->y() },
        { matrix->col0()->z(), matrix->col1()->z(), matrix->col2()->z(), matrix->col3()->z() },
        { matrix->col0()->w(), matrix->col1()->w(), matrix->col2()->w(), matrix->col3()->w() }
    };

        this->position = {this->matrix[3].x, this->matrix[3].y, this->matrix[3].z};
    Camera::INSTANCE()->adjustPositionIfInThirdPersonMode(this);

    this->dirty = true;
}

void Renderable::setMatrixForBoundingSphere(const BoundingSphere sphere)
{
    this->matrix = {
        {1,0,0,0},
        {0,1,0,0},
        {0,0,1,0},
        {sphere.center.x, sphere.center.y, sphere.center.z, 1}
    };
}


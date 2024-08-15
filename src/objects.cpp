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

const std::string Renderable::getId() const
{
    return this->id;
}

void Renderable::setMatrix(const Matrix * matrix) {
    this->matrix =
    {
        { matrix->col0()->x(), matrix->col0()->y(), matrix->col0()->z(), matrix->col0()->w() },
        { matrix->col1()->x(), matrix->col1()->y(), matrix->col1()->z(), matrix->col1()->w() },
        { matrix->col2()->x(), matrix->col2()->y(), matrix->col2()->z(), matrix->col2()->w() },
        { matrix->col3()->x(), matrix->col3()->y(), matrix->col3()->z(), matrix->col3()->w() }
    };

    this->dirty = true;
}

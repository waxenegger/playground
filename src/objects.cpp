#include "includes/objects.h"

Renderable::Renderable() {}

Renderable::~Renderable() {}

void Renderable::setInactive(const bool & inactive) {
    this->inactive = inactive;
}

void Renderable::setDirty(const bool & dirty) {
    this->dirty = dirty;
}

void Renderable::flagAsRegistered() {
    this->registered = true;
}

bool Renderable::hasBeenRegistered() {
    return this->registered;
}

bool Renderable::shouldBeRendered(const std::array<glm::vec4, 6> & frustumPlanes) const
{
    return !this->inactive && this->isInFrustum(frustumPlanes);
}

void Renderable::setPosition(const glm::vec3 & position) {
    if (position == this->position) return;

    const glm::mat4 oldMatrix = this->matrix;

    this->position = position;
    this->updateMatrix();

    this->updateBbox(oldMatrix);
}

const BoundingBox & Renderable::getBoundingBox() const
{
    return this->bbox;
}

void Renderable::setScaling(const float & factor) {
    if (factor <= 0 || factor == this->scaling) return;

    const glm::mat4 oldMatrix = this->matrix;

    this->scaling = factor;
    this->updateMatrix();

    this->updateBbox(oldMatrix);
}

void Renderable::setRotation(glm::vec3 & rotation) {
    if (rotation == this->rotation) return;

    const glm::mat4 oldMatrix = this->matrix;

    this->rotation = rotation;
    this->updateMatrix();

    this->updateBbox(oldMatrix);
}


const glm::vec3 Renderable::getPosition() const {
    return this->position;
}

const float Renderable::getScaling() const {
    return this->scaling;
}

const glm::mat4 Renderable::getMatrix() const
{
    return this->matrix;
}

bool Renderable::isInFrustum(const std::array<glm::vec4, 6> & frustumPlanes) const {
    if (this->bbox.min[0] == INF && this->bbox.max[0] == NEG_INF) return true;

    // TODO: revisit bounding box
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
}

void Renderable::updateBbox(const glm::mat4 & oldMatrix) {
    if (oldMatrix == glm::mat4(0)) return;

    const glm::mat4 invMatrix = glm::inverse(oldMatrix);
    glm::vec4 prevMin = invMatrix * glm::vec4(this->bbox.min, 1.0);
    glm::vec4 prevMax = invMatrix * glm::vec4(this->bbox.max, 1.0);

    glm::vec3 minTransformed =  this->matrix * prevMin;
    glm::vec3 maxTransformed =  this->matrix * prevMax;

    // TODO: rotation is not really well covered

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

ColorVerticesRenderable::ColorVerticesRenderable() : Renderable(){}

ColorVerticesRenderable::ColorVerticesRenderable(const ColorVertexGeometry & geometry) : vertices(geometry.vertices),indices(geometry.indices) {
    this->bbox = geometry.bbox;
}

void ColorVerticesRenderable::setVertices(const std::vector<ColorVertex> & vertices)
{
    this->vertices = vertices;
}

const std::vector<ColorVertex> & ColorVerticesRenderable::getVertices() const
{
    return this->vertices;
}

void ColorVerticesRenderable::setIndices(const std::vector<uint32_t> & indices)
{
    this->indices = indices;
}

const std::vector<uint32_t> & ColorVerticesRenderable::getIndices() const
{
    return this->indices;
}

ColorMeshRenderable::ColorMeshRenderable() : Renderable() {}

ColorMeshRenderable::ColorMeshRenderable(const ColorMeshGeometry& geometry) : meshes(geometry.meshes)
{
    this->bbox = geometry.bbox;
}

void ColorMeshRenderable::setMeshes(const std::vector<VertexMesh>& meshes)
{
    this->meshes = meshes;
}

const std::vector<VertexMesh> & ColorMeshRenderable::getMeshes() const
{
    return this->meshes;
}

StaticColorVerticesRenderable::StaticColorVerticesRenderable() : ColorVerticesRenderable(){}
StaticColorVerticesRenderable::StaticColorVerticesRenderable(const ColorVertexGeometry & geometry) : ColorVerticesRenderable(geometry) {}
// static ones cannot be modified
void StaticColorVerticesRenderable::setPosition(const glm::vec3 & position) {
    logInfo("Static Objects cannot be modified");
}
void StaticColorVerticesRenderable::setScaling(const float & factor) {
    logInfo("Static Objects cannot be modified");
}
void StaticColorVerticesRenderable::setRotation(glm::vec3 & rotation) {
    logInfo("Static Objects cannot be modified");
};
void StaticColorVerticesRenderable::move(const float delta, const Direction & direction) {
    logInfo("Static Objects cannot be modified");
}
void StaticColorVerticesRenderable::rotate(int xAxis, int yAxis, int zAxis) {
    logInfo("Static Objects cannot be modified");
}

StaticColorMeshRenderable::StaticColorMeshRenderable() : ColorMeshRenderable(){}
StaticColorMeshRenderable::StaticColorMeshRenderable(const ColorMeshGeometry & geometry) : ColorMeshRenderable(geometry) {}
// static ones cannot be modified
void StaticColorMeshRenderable::setPosition(const glm::vec3 & position) {
    logInfo("Static Objects cannot be modified");
}
void StaticColorMeshRenderable::setScaling(const float & factor) {
    logInfo("Static Objects cannot be modified");
}
void StaticColorMeshRenderable::setRotation(glm::vec3 & rotation) {
    logInfo("Static Objects cannot be modified");
};
void StaticColorMeshRenderable::move(const float delta, const Direction & direction) {
    logInfo("Static Objects cannot be modified");
}
void StaticColorMeshRenderable::rotate(int xAxis, int yAxis, int zAxis) {
    logInfo("Static Objects cannot be modified");
}

DynamicColorVerticesRenderable::DynamicColorVerticesRenderable() : ColorVerticesRenderable(){}
DynamicColorVerticesRenderable::DynamicColorVerticesRenderable(const ColorVertexGeometry & geometry) : ColorVerticesRenderable(geometry) {}

DynamicColorMeshRenderable::DynamicColorMeshRenderable() : ColorMeshRenderable(){}
DynamicColorMeshRenderable::DynamicColorMeshRenderable(const ColorMeshGeometry & geometry) : ColorMeshRenderable(geometry) {}

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

void GlobalRenderableStore::registerRenderable(Renderable * renderableObject)
{
    this->objects.push_back(std::unique_ptr<Renderable>(renderableObject));
    renderableObject->flagAsRegistered();
}

GlobalRenderableStore * GlobalRenderableStore::instance = nullptr;


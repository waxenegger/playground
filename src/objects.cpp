#include "includes/objects.h"

Renderable::Renderable() {}

Renderable::~Renderable() {}

void Renderable::setCulled(const bool & culled) {
    this->culled = culled;
}

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

bool Renderable::shouldBeRendered() const
{
    return !this->culled && !this->inactive;
}

StaticColorVerticesRenderable::StaticColorVerticesRenderable() : Renderable(){}

StaticColorVerticesRenderable::StaticColorVerticesRenderable(const std::vector<ColorVertex> & vertices) : vertices(vertices) {}

StaticColorVerticesRenderable::StaticColorVerticesRenderable(const std::vector<ColorVertex> & vertices, const std::vector<uint32_t> & indices) : vertices(vertices), indices(indices) {}


void StaticColorVerticesRenderable::setVertices(const std::vector<ColorVertex> & vertices)
{
    this->vertices = vertices;
}

const std::vector<ColorVertex> & StaticColorVerticesRenderable::getVertices() const
{
    return this->vertices;
}

void StaticColorVerticesRenderable::setIndices(const std::vector<uint32_t> & indices)
{
    this->indices = indices;
}

const std::vector<uint32_t> & StaticColorVerticesRenderable::getIndices() const
{
    return this->indices;
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

void GlobalRenderableStore::registerRenderable(Renderable * renderableObject)
{
    this->objects.push_back(std::unique_ptr<Renderable>(renderableObject));
    renderableObject->flagAsRegistered();
}

GlobalRenderableStore * GlobalRenderableStore::instance = nullptr;


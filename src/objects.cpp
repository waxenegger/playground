#include "includes/objects.h"

void Renderable::setCulled(const bool & culled) {
    this->culled = culled;
}

void Renderable::setInactive(const bool & inactive) {
    this->inactive = inactive;
}

void Renderable::setDirty(const bool & dirty) {
    this->dirty = dirty;
}

bool Renderable::shouldBeRendere() const
{
    return !this->culled && !this->inactive;
}

void StaticColorVerticesRenderable::setColorVertices(const std::vector<ColorVertex> & vertices)
{
    this->vertices = vertices;
}

const std::vector<ColorVertex> & StaticColorVerticesRenderable::getColorVertices() const
{
    return this->vertices;
}


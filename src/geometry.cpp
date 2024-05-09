#include "includes/geometry.h"

BoundingBox Geometry::getBoundingBox(const glm::vec3 pos, const float buffer) {
    return BoundingBox {
        .min = glm::vec3(pos.x-buffer, pos.y-buffer, pos.z-buffer),
        .max = glm::vec3(pos.x+buffer, pos.y+buffer, pos.z+buffer)
    };
}

bool Geometry::checkBBoxIntersection(const BoundingBox & bbox1, const BoundingBox & bbox2) {
    const bool intersectsAlongX =
        (bbox1.min.x >= bbox2.min.x && bbox1.min.x <= bbox2.max.x) ||
        (bbox1.max.x >= bbox2.min.x && bbox1.max.x <= bbox2.max.x) ||
        (bbox1.min.x <= bbox2.min.x && bbox1.max.x >= bbox2.max.x);
    if (!intersectsAlongX) return false;

    const bool intersectsAlongY =
        (bbox1.min.y >= bbox2.min.y && bbox1.min.y <= bbox2.max.y) ||
        (bbox1.max.y >= bbox2.min.y && bbox1.max.y <= bbox2.max.y) ||
        (bbox1.min.y <= bbox2.min.y && bbox1.max.y >= bbox2.max.y);
    if (!intersectsAlongY) return false;

    const bool intersectsAlongZ =
        (bbox1.min.z >= bbox2.min.z && bbox1.min.z <= bbox2.max.z) ||
        (bbox1.max.z >= bbox2.min.z && bbox1.max.z <= bbox2.max.z) ||
        (bbox1.min.z <= bbox2.min.z && bbox1.max.z >= bbox2.max.z);
    if (!intersectsAlongZ) return false;

    return true;
}

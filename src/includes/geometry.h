#ifndef SRC_INCLUDES_GEOMETRY_INCL_H_
#define SRC_INCLUDES_GEOMETRY_INCL_H_

#include "shared.h"

struct BoundingBox final {
    glm::vec3 min = glm::vec3(INF);
    glm::vec3 max = glm::vec3(NEG_INF);
    glm::vec3 center = glm::vec3(0);
    float radius = 0.0f;
};

struct ColorVertex final {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

class Geometry final {
    public:
        Geometry(const Geometry&) = delete;
        Geometry& operator=(const Geometry &) = delete;
        Geometry(Geometry &&) = delete;
        Geometry & operator=(Geometry) = delete;

        static bool  checkBBoxIntersection(const BoundingBox & bbox1, const BoundingBox & bbox2);
        static BoundingBox getBoundingBox(const glm::vec3 pos, const float buffer = 0.15f);
};


#endif



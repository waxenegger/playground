#ifndef SRC_INCLUDES_GEOMETRY_INCL_H_
#define SRC_INCLUDES_GEOMETRY_INCL_H_

#include "objects.h"

class Geometry final {
    public:
        Geometry(const Geometry&) = delete;
        Geometry& operator=(const Geometry &) = delete;
        Geometry(Geometry &&) = delete;
        Geometry & operator=(Geometry) = delete;

        static bool  checkBBoxIntersection(const BoundingBox & bbox1, const BoundingBox & bbox2);
        static BoundingBox getBoundingBox(const glm::vec3 pos, const float buffer = 0.15f);
        static std::unique_ptr<ColorMeshGeometry> createSphereColorMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec4 & color = glm::vec4(1.0f));

        static std::unique_ptr<ColorMeshGeometry>  createBoxColorMeshGeometry(const float & width, const float & height, const float & depth, const glm::vec4 & color = glm::vec4(1.0f));

        static std::unique_ptr<ColorMeshRenderable> getNormalsFromColorMeshRenderables(const std::vector<ColorMeshRenderable *> & source, const glm::vec3 & color = { 1.0f, 0.0f, 0.0f });

        static std::unique_ptr<ColorMeshRenderable> getBboxesFromRenderables(const std::vector<Renderable *> & source, const glm::vec3 & color = { 0.0f, 0.0f, 1.0f });
};


#endif



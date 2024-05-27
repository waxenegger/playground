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
        static ColorVertexGeometry createSphereColorVertexGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec4 & color = glm::vec4(1.0f));
        static ColorMeshGeometry createSphereColorMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec4 & color = glm::vec4(1.0f));

        static ColorVertexGeometry createBoxColorVertexGeometry(const float & width, const float & height, const float & depth, const glm::vec4 & color = glm::vec4(1.0f));
        static ColorMeshGeometry createBoxColorMeshGeometry(const float & width, const float & height, const float & depth, const glm::vec4 & color = glm::vec4(1.0f));

        template<typename T>
        static void getNormalsFromColorVertexRenderables(const std::vector<T *> & source, std::vector<StaticColorVerticesRenderable *> & dest, const glm::vec3 color = { 1.0f, 0.0f, 0.0f }) {
            if (source.empty()) return;

            std::vector<ColorVertex> lines;
            for (const auto & o : source) {
                for (const auto & v : o->getVertices()) {
                    const glm::vec3 transformedPosition = o->getMatrix() * glm::vec4(v.position, 1.0f);
                    const glm::vec3 lengthAdjustedNormal = o->getMatrix() * glm::vec4(v.position + glm::normalize(v.normal) * 0.25f, 1);

                    lines.push_back( {{transformedPosition,transformedPosition}, color} );
                    lines.push_back( {{lengthAdjustedNormal, lengthAdjustedNormal}, color} );
                }
            }

            auto normalsObject = new StaticColorVerticesRenderable(ColorVertexGeometry {lines});
            GlobalRenderableStore::INSTANCE()->registerRenderable(normalsObject);
            dest.push_back(normalsObject);
        };

        template<typename T>
        static void getBboxesFromColorVertexRenderables(const std::vector<T *> & source, std::vector<StaticColorVerticesRenderable *> & dest, const glm::vec3 color = { 0.0f, 0.0f, 1.0f })
        {
            if (source.empty()) return;

            std::vector<ColorVertex> lines;
            for (const auto & o : source) {
                const auto & l = Helper::getBboxWireframeAsColorVertexLines(o->getBoundingBox(), color);
                lines.insert(lines.end(), l.begin(), l.end());
            }

            auto bboxesObject = new StaticColorVerticesRenderable (ColorVertexGeometry { lines });
            GlobalRenderableStore::INSTANCE()->registerRenderable(bboxesObject);
            dest.push_back(bboxesObject);
        }
};


#endif



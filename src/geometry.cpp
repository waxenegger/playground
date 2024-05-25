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

ColorVertexGeometry Geometry::createSphere(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec3 & color)
{
    ColorVertexGeometry geom;
    geom.bbox.radius = radius;
    geom.bbox.center = {0,0,0};
    geom.bbox.min = { -radius, -radius, -radius};
    geom.bbox.max = { radius, radius, radius};

    const float radDiv = 1 / radius;

    const auto & top = ColorVertex { glm::vec3(0.0f, radius, 0.0f), glm::vec3(0.0f, 1 , 0.0f), color };
    const auto & bottom = ColorVertex { glm::vec3(0.0f, -radius, 0.0f), glm::vec3(0.0f, -1, 0.0f), color };

    float deltaLon = 2 * glm::pi<float>() / (lonIntervals < 5 ? 5 : lonIntervals);
    float deltaLat = glm::pi<float>() / (latIntervals < 5 ? latIntervals : latIntervals);

    float theta = deltaLat;

    // do top vertex and indices
    geom.vertices.push_back(top);
    uint32_t j=1;
    while (j<lonIntervals) {
        geom.indices.push_back(0);
        geom.indices.push_back(j);
        geom.indices.push_back(j+1);
        j++;
    }
    geom.indices.push_back(0);
    geom.indices.push_back(j);
    geom.indices.push_back(1);
    uint32_t lonOffset = 1;

    while (theta < glm::pi<float>()) {
        float phi = 0;
        bool isBottom = theta + deltaLat >= glm::pi<float>();
        bool isTop = theta == deltaLat;

        if (isBottom) {
            geom.vertices.push_back(bottom);
        } else {
            while (phi < 2 * glm::pi<float>()) {
                const glm::vec3 v = glm::vec3(
                    radius * glm::sin(theta) * glm::cos(phi),
                    radius * glm::cos(theta),
                    radius * glm::sin(theta) * glm::sin(phi)
                );
                const auto & vert = ColorVertex { v, v * radDiv, color };
                geom.vertices.push_back(vert);

                phi += deltaLon;
            }

            if (!isTop) lonOffset += lonIntervals;
        }

        if (isBottom) {
            j = lonOffset;
            geom.indices.push_back(j);
            geom.indices.push_back(geom.vertices.size()-2);
            geom.indices.push_back(geom.vertices.size()-1);
            while (j<lonOffset+lonIntervals) {
                geom.indices.push_back(j);
                geom.indices.push_back(geom.vertices.size()-1);
                geom.indices.push_back(j+1);
                j++;
            }
        } else if (!isTop) {
            while (j<lonOffset+lonIntervals) {
                geom.indices.push_back(j-lonIntervals+1);
                geom.indices.push_back(j-lonIntervals);
                geom.indices.push_back(j);
                geom.indices.push_back(j-lonIntervals+1);
                geom.indices.push_back(j);
                geom.indices.push_back(j+1);
                j++;
            }
        }

        theta += deltaLat;
    }

    return geom;
}

ColorVertexGeometry Geometry::createBox(const float& width, const float& height, const float& depth, const glm::vec3& color)
{
    ColorVertexGeometry geom;

    const auto & middle = glm::vec3 {width, height, depth} * .5f;
    geom.bbox.center = middle;
    geom.bbox.min = { -middle.x, -middle.y, -middle.z };
    geom.bbox.max = { middle.x, middle.y, middle.z };

    const float len = glm::sqrt(middle.x * middle.x + middle.y * middle.y + middle.z * middle.z);
    geom.bbox.radius = len;

    geom.vertices.push_back({ { middle.x, middle.y, middle.z  }, glm::vec3 { middle.x, middle.y, middle.z  } / len, color });
    geom.vertices.push_back({ { middle.x, -middle.y, middle.z }, glm::vec3 { middle.x, -middle.y, middle.z  } / len, color });
    geom.vertices.push_back({ { middle.x, -middle.y, -middle.z }, glm::vec3 { middle.x,-middle.y, -middle.z  } / len, color });
    geom.vertices.push_back({ { middle.x, middle.y, -middle.z  }, glm::vec3 { middle.x, middle.y, -middle.z  } / len, color });
    geom.vertices.push_back({ { -middle.x, -middle.y, -middle.z  }, glm::vec3 { -middle.x, -middle.y, -middle.z  } / len, color });
    geom.vertices.push_back({ { -middle.x, -middle.y, middle.z  }, glm::vec3 { -middle.x, -middle.y, middle.z  } / len, color });
    geom.vertices.push_back({ { -middle.x, middle.y, middle.z  }, glm::vec3 { -middle.x, middle.y, middle.z  } / len, color });
    geom.vertices.push_back({ { -middle.x, middle.y, -middle.z  }, glm::vec3 { -middle.x, middle.y, -middle.z  } / len, color });

    geom.indices = {
        7, 4, 2, 2, 3, 7,
        5, 4, 7, 7, 6, 5,
        2, 1, 0, 0, 3, 2,
        5, 6, 0, 0, 1, 5,
        7, 3, 0, 0, 6, 7,
        4, 5, 2, 2, 5, 1
    };


    return geom;
}

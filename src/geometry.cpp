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

std::pair<std::vector<ColorVertex>, std::vector<uint32_t>> Geometry::createSphere(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec3 & color)
{
    std::vector<ColorVertex> vertices;
    std::vector<uint32_t> indices;

    const float radDiv = 1 / radius;

    const auto & top = ColorVertex { glm::vec3(0.0f, radius, 0.0f), glm::vec3(0.0f, 1 , 0.0f), color };
    const auto & bottom = ColorVertex { glm::vec3(0.0f, -radius, 0.0f), glm::vec3(0.0f, -1, 0.0f), color };

    float deltaLon = 2 * glm::pi<float>() / (lonIntervals < 5 ? 5 : lonIntervals);
    float deltaLat = glm::pi<float>() / (latIntervals < 5 ? latIntervals : latIntervals);

    float theta = deltaLat;
    uint32_t lonOffset = 0;

    vertices.push_back(top);

    while (theta < glm::pi<float>()) {
        float phi = 0;
        bool isBottom = theta + deltaLat >= glm::pi<float>();
        bool isTop = theta == deltaLat;

        while (phi < 2* glm::pi<float>()) {
            if (isBottom) {
                vertices.push_back(bottom);
            } else {
                const glm::vec3 v = glm::vec3(
                    radius * glm::sin(theta) * glm::cos(phi),
                    radius * glm::cos(theta),
                    radius * glm::sin(theta) * glm::sin(phi)
                );
                const auto & vert = ColorVertex { v, v * radDiv, color };
                vertices.push_back(vert);

                if (isTop) {
                    lonOffset = lonIntervals+1;
                    uint32_t j=1;

                    while (j<lonIntervals) {
                        indices.push_back(j);
                        indices.push_back(j+1);
                        indices.push_back(0);
                        j++;
                    }

                    indices.push_back(j);
                    indices.push_back(1);
                    indices.push_back(0);
                } else {
                    uint32_t j=lonOffset-1;
                    while (j<lonOffset+lonIntervals) {
                        indices.push_back(j-lonIntervals+1);
                        indices.push_back(j-lonIntervals);
                        indices.push_back(j);
                        indices.push_back(j-lonIntervals+1);
                        indices.push_back(j);
                        indices.push_back(j+1);
                        j++;
                    }

                    lonOffset += lonIntervals;
                }
            }

            phi += deltaLon;
         }

         theta += deltaLat;
    }

    return std::make_pair(vertices, indices);
}

std::pair<std::vector<ColorVertex>, std::vector<uint32_t>> Geometry::createBox(const float& width, const float& height, const float& depth, const glm::vec3& color)
{
    const auto & middle = glm::vec3 {width, height, depth} * .5f;
    const float len = glm::sqrt(middle.x * middle.x + middle.y * middle.y + middle.z * middle.z);

    std::vector<ColorVertex> vertices;
    vertices.push_back({ { middle.x, middle.y, middle.z  }, glm::vec3 { middle.x, middle.y, middle.z  } / len, color });
    vertices.push_back({ { middle.x, -middle.y, middle.z }, glm::vec3 { middle.x, -middle.y, middle.z  } / len, color });
    vertices.push_back({ { middle.x, -middle.y, -middle.z }, glm::vec3 { middle.x,-middle.y, -middle.z  } / len, color });
    vertices.push_back({ { middle.x, middle.y, -middle.z  }, glm::vec3 { middle.x, middle.y, -middle.z  } / len, color });
    vertices.push_back({ { -middle.x, -middle.y, -middle.z  }, glm::vec3 { -middle.x, -middle.y, -middle.z  } / len, color });
    vertices.push_back({ { -middle.x, -middle.y, middle.z  }, glm::vec3 { -middle.x, -middle.y, middle.z  } / len, color });
    vertices.push_back({ { -middle.x, middle.y, middle.z  }, glm::vec3 { -middle.x, middle.y, middle.z  } / len, color });
    vertices.push_back({ { -middle.x, middle.y, -middle.z  }, glm::vec3 { -middle.x, middle.y, -middle.z  } / len, color });

    std::vector<uint32_t> indices = {
        7, 4, 2, 2, 3, 7,
        5, 4, 7, 7, 6, 5,
        2, 1, 0, 0, 3, 2,
        5, 6, 0, 0, 1, 5,
        7, 3, 0, 0, 6, 7,
        4, 5, 2, 2, 5, 1
    };


    return std::make_pair(vertices, indices);
}

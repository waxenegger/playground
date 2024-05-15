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

std::vector<ColorVertex> Geometry::createSphere(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec3 & color)
{
    std::vector<ColorVertex> vertices;

    auto top = ColorVertex { glm::vec3(0.0f, radius, 0.0f), glm::vec3(0.0f, radius, 0.0f), color };
    const auto & bottom = ColorVertex { glm::vec3(0.0f, -radius, 0.0f), glm::vec3(0.0f, -radius, 0.0f), color };

    float deltaLon = 2 * glm::pi<float>() / (lonIntervals < 5 ? 5 : lonIntervals);
    float deltaLat = glm::pi<float>() / (latIntervals < 5 ? latIntervals : latIntervals);
    const float radDiv = 1 / radius;

    float theta = deltaLat;

    while (theta < glm::pi<float>()) {
        float phi = 0;
        bool isBottom = theta + deltaLat >= glm::pi<float>();
        bool isTop = theta == deltaLat;

        while (phi < 2* glm::pi<float>()) {

            if (!isTop) {
                const glm::vec3 & topV = glm::vec3(
                    radius * glm::sin(theta-deltaLat) * glm::cos(phi),
                    radius * glm::cos(theta-deltaLat),
                    radius * glm::sin(theta-deltaLat) * glm::sin(phi)
                );
                top = ColorVertex { topV, topV * radius, color };

            }
            vertices.push_back(top);

            ColorVertex vert2;
            if (!isBottom) {
                const glm::vec3 v = glm::vec3(
                    radius * glm::sin(theta) * glm::cos(phi),
                    radius * glm::cos(theta),
                    radius * glm::sin(theta) * glm::sin(phi)
                );
                const auto & vert = ColorVertex { v, v * radDiv, color };
                vertices.push_back(vert);

                const glm::vec3 v2 = glm::vec3(
                    radius * glm::sin(theta) * glm::cos(phi+deltaLon),
                    radius * glm::cos(theta),
                    radius * glm::sin(theta) * glm::sin(phi+deltaLon)
                );
                vert2 = ColorVertex { v2, v2 * radDiv, color };
                vertices.push_back(vert2);
            }

            if (!isTop || isBottom) {
                if (isBottom) vertices.push_back(bottom);
                if (!isBottom) vertices.push_back(vert2);

                const glm::vec3 & topV = glm::vec3(
                    radius * glm::sin(theta-deltaLat) * glm::cos(phi+deltaLon),
                    radius * glm::cos(theta-deltaLat),
                    radius * glm::sin(theta-deltaLat) * glm::sin(phi+deltaLon)
                );
                const auto & topVert = ColorVertex {topV, topV * radDiv, color };
                vertices.push_back(topVert);

                if (!isBottom) vertices.push_back(top);
            }

             phi += deltaLon;
         }

         theta += deltaLat;
    }

    return vertices;
}

std::vector<ColorVertex> Geometry::createBox(const float& width, const float& height, const float& depth, const glm::vec3& color)
{
    std::vector<ColorVertex> vertices;

    const auto & middle = glm::vec3 {width, height, depth} * .5f;
    const float len = glm::sqrt(middle.x * middle.x + middle.y * middle.y + middle.z * middle.z);

    vertices.push_back({ {  middle.x, middle.y, -middle.z  }, glm::vec3  {  middle.x, middle.y, -middle.z  } / len, color });
    vertices.push_back({ { -middle.x, middle.y, -middle.z  }, glm::vec3 { -middle.x, middle.y, -middle.z  } / len, color });
    vertices.push_back({ { -middle.x, middle.y, middle.z  },  glm::vec3 { -middle.x, middle.y, middle.z  } / len, color });

    vertices.push_back({ { -middle.x, middle.y, middle.z  },  glm::vec3 { -middle.x, middle.y, middle.z  } / len, color });
    vertices.push_back({ {  middle.x, middle.y, middle.z  },  glm::vec3 {  middle.x, middle.y, middle.z  } / len, color });
    vertices.push_back({ {  middle.x, middle.y, -middle.z  }, glm::vec3 { middle.x, middle.y, -middle.z  } / len, color });

    vertices.push_back({ { -middle.x, middle.y, middle.z  },   glm::vec3 { -middle.x, middle.y, middle.z  } / len, color });
    vertices.push_back({ { -middle.x, middle.y, -middle.z  },  glm::vec3 { -middle.x, middle.y, -middle.z  } / len, color });
    vertices.push_back({ { -middle.x, -middle.y, -middle.z  }, glm::vec3 { -middle.x, -middle.y, -middle.z  } / len, color });

    vertices.push_back({ { -middle.x, -middle.y, -middle.z  }, glm::vec3 { -middle.x, -middle.y, -middle.z  } / len, color });
    vertices.push_back({ { -middle.x, -middle.y, middle.z  },  glm::vec3 { -middle.x, -middle.y, middle.z  } / len, color });
    vertices.push_back({ { -middle.x, middle.y, middle.z  },   glm::vec3 { -middle.x, middle.y, middle.z  } / len, color });

    vertices.push_back({ { middle.x, -middle.y, -middle.z  }, glm::vec3 { middle.x, -middle.y, -middle.z  } / len, color });
    vertices.push_back({ { middle.x, middle.y, -middle.z  },  glm::vec3 { middle.x, middle.y, -middle.z  } / len, color });
    vertices.push_back({ { middle.x, middle.y, middle.z  },   glm::vec3 { middle.x, middle.y, middle.z  } / len, color });

    vertices.push_back({ { middle.x, middle.y, middle.z  },   glm::vec3 { middle.x, middle.y, middle.z  } / len, color });
    vertices.push_back({ { middle.x, -middle.y, middle.z  },  glm::vec3 { middle.x, -middle.y, middle.z  } / len, color });
    vertices.push_back({ { middle.x, -middle.y, -middle.z  }, glm::vec3 { middle.x, -middle.y, -middle.z  } / len, color });

    vertices.push_back({ { -middle.x, -middle.y, -middle.z  }, glm::vec3 { -middle.x, -middle.y, -middle.z  } / len, color });
    vertices.push_back({ { -middle.x, middle.y, -middle.z  },  glm::vec3 { -middle.x, middle.y, -middle.z  } / len, color });
    vertices.push_back({ { middle.x, middle.y, -middle.z  },   glm::vec3 { middle.x, middle.y, -middle.z  } / len, color });

    vertices.push_back({ { middle.x, -middle.y, -middle.z  },  glm::vec3 { middle.x, -middle.y, -middle.z  } / len, color });
    vertices.push_back({ { -middle.x, -middle.y, -middle.z  }, glm::vec3 { -middle.x,-middle.y, -middle.z  } / len, color });
    vertices.push_back({ { middle.x, middle.y, -middle.z  },   glm::vec3 { middle.x, middle.y, -middle.z  } / len, color });

    vertices.push_back({ { -middle.x, -middle.y, middle.z  },   glm::vec3 { -middle.x, -middle.y, middle.z  } / len, color });
    vertices.push_back({ { middle.x, -middle.y, middle.z  },  glm::vec3 { middle.x, -middle.y, middle.z  } / len, color });
    vertices.push_back({ { middle.x, middle.y, middle.z  }, glm::vec3 { middle.x, middle.y, middle.z  } / len, color });

    vertices.push_back({ { -middle.x, -middle.y, middle.z  },   glm::vec3 { -middle.x, -middle.y, middle.z  } / len, color });
    vertices.push_back({ { middle.x, middle.y, middle.z  }, glm::vec3 { middle.x,middle.y, middle.z  } / len, color });
    vertices.push_back({ { -middle.x, middle.y, middle.z  },  glm::vec3 { -middle.x, middle.y, middle.z  } / len, color });

    vertices.push_back({ { -middle.x, -middle.y, middle.z  },  glm::vec3 { -middle.x, -middle.y, middle.z  }  / len, color });
    vertices.push_back({ { -middle.x, -middle.y, -middle.z  }, glm::vec3 { -middle.x, -middle.y, -middle.z  } / len, color });
    vertices.push_back({ {  middle.x, -middle.y, -middle.z  }, glm::vec3 {  middle.x, -middle.y, -middle.z  } / len, color });

    vertices.push_back({ {  middle.x, -middle.y, -middle.z  }, glm::vec3 { middle.x, -middle.y, -middle.z  } / len, color });
    vertices.push_back({ {  middle.x, -middle.y, middle.z  },  glm::vec3 {  middle.x, -middle.y, middle.z  } / len, color });
    vertices.push_back({ { -middle.x, -middle.y, middle.z  },  glm::vec3 { -middle.x, -middle.y, middle.z  } / len, color });

    return vertices;
}



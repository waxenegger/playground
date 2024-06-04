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

std::unique_ptr<ColorMeshGeometry> Geometry::createSphereColorMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec4 & color) {
    auto geom = std::make_unique<ColorMeshGeometry>();
    geom->bbox.radius = radius;
    geom->bbox.center = {0,0,0};
    geom->bbox.min = { -radius, -radius, -radius};
    geom->bbox.max = { radius, radius, radius};

    const float radDiv = 1 / radius;

    const auto & top = Vertex { glm::vec3(0.0f, radius, 0.0f), glm::vec3(0.0f, 1 , 0.0f) };
    const auto & bottom = Vertex { glm::vec3(0.0f, -radius, 0.0f), glm::vec3(0.0f, -1, 0.0f) };

    float deltaLon = 2 * glm::pi<float>() / (lonIntervals < 5 ? 5 : lonIntervals);
    float deltaLat = glm::pi<float>() / (latIntervals < 5 ? latIntervals : latIntervals);

    float theta = deltaLat;

    VertexMesh mesh;
    mesh.color = color;

    // do top vertex and indices
    mesh.vertices.emplace_back(top);
    uint32_t j=1;
    while (j<lonIntervals) {
        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(j);
        mesh.indices.emplace_back(j+1);
        j++;
    }
    mesh.indices.emplace_back(0);
    mesh.indices.emplace_back(j);
    mesh.indices.emplace_back(1);
    uint32_t lonOffset = 1;

    while (theta < glm::pi<float>()) {
        float phi = 0;
        bool isBottom = theta + deltaLat >= glm::pi<float>();
        bool isTop = theta == deltaLat;

        if (isBottom) {
            mesh.vertices.emplace_back(bottom);
        } else {
            while (phi < 2 * glm::pi<float>()) {
                const glm::vec3 v = glm::vec3(
                    radius * glm::sin(theta) * glm::cos(phi),
                    radius * glm::cos(theta),
                    radius * glm::sin(theta) * glm::sin(phi)
                );
                const auto & vert = Vertex { v, v * radDiv};
                mesh.vertices.emplace_back(vert);

                phi += deltaLon;
            }

            if (!isTop) lonOffset += lonIntervals;
        }

        if (isBottom) {
            j = lonOffset;
            mesh.indices.emplace_back(j);
            mesh.indices.emplace_back(static_cast<uint32_t>(mesh.vertices.size())-2);
            mesh.indices.emplace_back(static_cast<uint32_t>(mesh.vertices.size())-1);
            while (j<lonOffset+lonIntervals) {
                mesh.indices.emplace_back(j);
                mesh.indices.emplace_back(mesh.vertices.size()-1);
                mesh.indices.emplace_back(j+1);
                j++;
            }
        } else if (!isTop) {
            while (j<lonOffset+lonIntervals) {
                mesh.indices.emplace_back(j-lonIntervals+1);
                mesh.indices.emplace_back(j-lonIntervals);
                mesh.indices.emplace_back(j);
                mesh.indices.emplace_back(j-lonIntervals+1);
                mesh.indices.emplace_back(j);
                mesh.indices.emplace_back(j+1);
                j++;
            }
        }

        theta += deltaLat;
    }

    geom->meshes.emplace_back(mesh);

    return geom;
}

std::unique_ptr<ColorMeshGeometry> Geometry::createBoxColorMeshGeometry(const float& width, const float& height, const float& depth, const glm::vec4& color)
{
    auto geom = std::make_unique<ColorMeshGeometry>();

    const auto & middle = glm::vec3 {width, height, depth} * .5f;
    geom->bbox.center = glm::vec3 {0.0f };
    geom->bbox.min = { -middle.x, -middle.y, -middle.z };
    geom->bbox.max = { middle.x, middle.y, middle.z };

    const float len = glm::sqrt(middle.x * middle.x + middle.y * middle.y + middle.z * middle.z);
    geom->bbox.radius = len;

    VertexMesh mesh;
    mesh.color = color;

    mesh.vertices.emplace_back(Vertex {{ middle.x, middle.y, middle.z  }, glm::vec3 { middle.x, middle.y, middle.z  } / len } );
    mesh.vertices.emplace_back(Vertex { { middle.x, -middle.y, middle.z }, glm::vec3 { middle.x, -middle.y, middle.z  } / len });
    mesh.vertices.emplace_back(Vertex { { middle.x, -middle.y, -middle.z }, glm::vec3 { middle.x,-middle.y, -middle.z  } / len });
    mesh.vertices.emplace_back(Vertex { { middle.x, middle.y, -middle.z  }, glm::vec3 { middle.x, middle.y, -middle.z  } / len });
    mesh.vertices.emplace_back(Vertex { { -middle.x, -middle.y, -middle.z  }, glm::vec3 { -middle.x, -middle.y, -middle.z  } / len });
    mesh.vertices.emplace_back(Vertex { { -middle.x, -middle.y, middle.z  }, glm::vec3 { -middle.x, -middle.y, middle.z  } / len });
    mesh.vertices.emplace_back(Vertex { { -middle.x, middle.y, middle.z  }, glm::vec3 { -middle.x, middle.y, middle.z  } / len });
    mesh.vertices.emplace_back(Vertex { { -middle.x, middle.y, -middle.z  }, glm::vec3 { -middle.x, middle.y, -middle.z  } / len });

    mesh.indices = {
        7, 4, 2, 2, 3, 7,
        5, 4, 7, 7, 6, 5,
        2, 1, 0, 0, 3, 2,
        5, 6, 0, 0, 1, 5,
        7, 3, 0, 0, 6, 7,
        4, 5, 2, 2, 5, 1
    };

    geom->meshes.emplace_back(mesh);

    return geom;
}

std::unique_ptr<ColorMeshGeometry> Geometry::getNormalsFromColorMeshRenderables(const std::vector<ColorMeshRenderable *> & source, const glm::vec3 & color)
{
    if (source.empty()) return nullptr;

    auto lines = std::make_unique<ColorMeshGeometry>();
    VertexMesh mesh;
    mesh.color = glm::vec4(color,1);

    for (const auto & o : source) {
        for (const auto & m : o->getMeshes()) {
            for (const auto & v : m.vertices) {
                const glm::vec3 transformedPosition = o->getMatrix() * glm::vec4(v.position, 1.0f);
                const glm::vec3 lengthAdjustedNormal = o->getMatrix() * glm::vec4(v.position + glm::normalize(v.normal) * 0.25f, 1);

                mesh.vertices.emplace_back(Vertex {transformedPosition,transformedPosition} );
                mesh.vertices.emplace_back(Vertex {lengthAdjustedNormal, lengthAdjustedNormal} );
            }
        }
    }

    lines->meshes.emplace_back(mesh);

    return lines;
}

std::unique_ptr<ColorMeshGeometry> Geometry::getBboxesFromRenderables(const std::vector<Renderable *> & source, const glm::vec3 & color)
{
    if (source.empty()) return nullptr;

    auto lines = std::make_unique<ColorMeshGeometry>();

    for (const auto & o : source) {
        const auto & l = Helper::getBboxWireframe(o->getBoundingBox());

        VertexMesh mesh;
        mesh.color = glm::vec4(color, 1);

        mesh.vertices = std::move(l);
        lines->meshes.emplace_back(mesh);
    }

    return lines;
}

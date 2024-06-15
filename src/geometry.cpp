#include "includes/helper.h"

std::vector<Vertex> Helper::getBboxWireframe(const BoundingBox & bbox) {
    std::vector<Vertex> lines;

    lines.push_back(Vertex {{bbox.min.x, bbox.min.y, bbox.min.z}, {bbox.min.x, bbox.min.y, bbox.min.z}});
    lines.push_back(Vertex {{bbox.max.x, bbox.min.y, bbox.min.z}, {bbox.max.x, bbox.min.y, bbox.min.z}});

    lines.push_back(Vertex {{bbox.min.x, bbox.min.y, bbox.min.z}, {bbox.min.x, bbox.min.y, bbox.min.z}});
    lines.push_back(Vertex {{bbox.min.x, bbox.max.y, bbox.min.z}, {bbox.min.x, bbox.max.y, bbox.min.z}});

    lines.push_back(Vertex {{bbox.min.x, bbox.min.y, bbox.min.z}, {bbox.min.x, bbox.min.y, bbox.min.z}});
    lines.push_back(Vertex {{bbox.min.x, bbox.min.y, bbox.max.z}, {bbox.min.x, bbox.min.y, bbox.max.z}});

    lines.push_back(Vertex {{bbox.max.x, bbox.min.y, bbox.min.z}, {bbox.max.x, bbox.min.y, bbox.min.z}});
    lines.push_back(Vertex {{bbox.max.x, bbox.max.y, bbox.min.z}, {bbox.max.x, bbox.max.y, bbox.min.z}});

    lines.push_back(Vertex {{bbox.min.x, bbox.max.y, bbox.min.z}, {bbox.min.x, bbox.max.y, bbox.min.z}});
    lines.push_back(Vertex {{bbox.min.x, bbox.max.y, bbox.max.z}, {bbox.min.x, bbox.max.y, bbox.max.z}});

    lines.push_back(Vertex {{bbox.min.x, bbox.max.y, bbox.min.z}, {bbox.min.x, bbox.max.y, bbox.min.z}});
    lines.push_back(Vertex {{bbox.max.x, bbox.max.y, bbox.min.z}, {bbox.max.x, bbox.max.y, bbox.min.z}});

    lines.push_back(Vertex {{bbox.min.x, bbox.min.y, bbox.max.z}, {bbox.min.x, bbox.min.y, bbox.max.z}});
    lines.push_back(Vertex {{bbox.max.x, bbox.min.y, bbox.max.z}, {bbox.max.x, bbox.min.y, bbox.max.z}});

    lines.push_back(Vertex {{bbox.max.x, bbox.min.y, bbox.max.z}, {bbox.max.x, bbox.min.y, bbox.max.z}});
    lines.push_back(Vertex {{bbox.max.x, bbox.max.y, bbox.max.z}, {bbox.max.x, bbox.max.y, bbox.max.z}});

    lines.push_back(Vertex {{bbox.max.x, bbox.max.y, bbox.max.z}, {bbox.max.x, bbox.max.y, bbox.max.z}});
    lines.push_back(Vertex {{bbox.max.x, bbox.max.y, bbox.min.z}, {bbox.max.x, bbox.max.y, bbox.min.z}});

    lines.push_back(Vertex {{bbox.max.x, bbox.max.y, bbox.max.z}, {bbox.max.x, bbox.max.y, bbox.max.z}});
    lines.push_back(Vertex {{bbox.min.x, bbox.max.y, bbox.max.z}, {bbox.min.x, bbox.max.y, bbox.max.z}});

    lines.push_back(Vertex {{bbox.min.x, bbox.max.y, bbox.max.z}, {bbox.min.x, bbox.max.y, bbox.max.z}});
    lines.push_back(Vertex {{bbox.min.x, bbox.min.y, bbox.max.z}, {bbox.min.x, bbox.min.y, bbox.max.z}});

    lines.push_back(Vertex {{bbox.max.x, bbox.min.y, bbox.max.z}, {bbox.max.x, bbox.min.y, bbox.max.z}});
    lines.push_back(Vertex {{bbox.max.x, bbox.min.y, bbox.min.z}, {bbox.max.x, bbox.min.y, bbox.min.z}});

    return lines;
}

BoundingBox Helper::createBoundingBoxFromMinMax(const glm::vec3 & mins, const glm::vec3 & maxs)
{
    BoundingBox bbox;
    bbox.min = mins;
    bbox.max = maxs;

    bbox.center.x = (bbox.max.x  + bbox.min.x) / 2;
    bbox.center.y = (bbox.max.y  + bbox.min.y) / 2;
    bbox.center.z = (bbox.max.z  + bbox.min.z) / 2;

    glm::vec3 distCorner = {
        bbox.min.x - bbox.center.x,
        bbox.min.y - bbox.center.y,
        bbox.min.z - bbox.center.z
    };

    bbox.radius = glm::sqrt(distCorner.x * distCorner.x + distCorner.y * distCorner.y + distCorner.z * distCorner.z);

    return bbox;
}

BoundingBox Helper::getBoundingBox(const glm::vec3 pos, const float buffer) {
    return BoundingBox {
        .min = glm::vec3(pos.x-buffer, pos.y-buffer, pos.z-buffer),
        .max = glm::vec3(pos.x+buffer, pos.y+buffer, pos.z+buffer)
    };
}

bool Helper::checkBBoxIntersection(const BoundingBox & bbox1, const BoundingBox & bbox2) {
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

std::unique_ptr<ColorMeshGeometry> Helper::createSphereColorMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec4 & color)
{
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

    VertexMeshIndexed mesh;
    mesh.color = color;

    // do top vertex and indices
    mesh.vertices.emplace_back(top);
    uint32_t j=1;
    while (j<=lonIntervals) {
        mesh.indices.emplace_back(j+1);
        mesh.indices.emplace_back(j);
        mesh.indices.emplace_back(0);
        j++;
    }

    uint32_t lonOffset = 1;
    while (theta < glm::pi<float>()) {
        float phi = 0;
        bool isBottom = glm::abs<float>(glm::pi<float>() - theta) < deltaLat;
        bool isTop = theta == deltaLat;

        if (isBottom) {
            mesh.vertices.emplace_back(bottom);
        } else {
            while (phi < 2 * glm::pi<float>() + deltaLon) {
                const glm::vec3 v = glm::vec3(
                    radius * glm::sin(theta) * glm::cos(phi),
                    radius * glm::cos(theta),
                    radius * glm::sin(theta) * glm::sin(phi)
                );
                const auto & vert = Vertex { v, v * radDiv};
                mesh.vertices.emplace_back(vert);

                phi += deltaLon;
            }

            if (!isTop) lonOffset += lonIntervals+1;
        }

        if (isBottom) {
            j = lonOffset;
            while (j<=lonOffset+lonIntervals) {
                mesh.indices.emplace_back(static_cast<uint32_t>(mesh.vertices.size())-1);
                mesh.indices.emplace_back(j);
                mesh.indices.emplace_back(j+1);
                j++;
            }
        } else if (!isTop) {
            while (j<=lonOffset+lonIntervals) {
                mesh.indices.emplace_back(j);
                mesh.indices.emplace_back(j-lonIntervals-1);
                mesh.indices.emplace_back(j-lonIntervals);
                mesh.indices.emplace_back(j+1);
                mesh.indices.emplace_back(j);
                mesh.indices.emplace_back(j-lonIntervals);
                j++;
            }
        }

        theta += deltaLat;
    }

    geom->meshes.emplace_back(mesh);

    return geom;
}

std::unique_ptr<TextureMeshGeometry> Helper::createSphereTextureMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const std::string & textureName)
{
    const auto texture = GlobalTextureStore::INSTANCE()->getTextureByName(textureName);
    if (texture == nullptr) {
        logError("Please provide an existing Texture from the store for the Mesh Geometry");
        return nullptr;
    }

    if (!texture->isValid()) {
        logError("Could not load Texture: " + texture->getPath() + " for Sphere Geometry");
        return nullptr;
    }

    auto geom = std::make_unique<TextureMeshGeometry>();
    geom->bbox.radius = radius;
    geom->bbox.center = {0,0,0};
    geom->bbox.min = { -radius, -radius, -radius};
    geom->bbox.max = { radius, radius, radius};

    const float radDiv = 1 / radius;

    const auto & top = TextureVertex { { glm::vec3(0.0f, radius, 0.0f), glm::vec3(0.0f, 1 , 0.0f) }, glm::vec2(0.5f, 0.0f) };
    const auto & bottom = TextureVertex { { glm::vec3(0.0f, -radius, 0.0f), glm::vec3(0.0f, -1, 0.0f) }, glm::vec2(0.5f, 1.0f) };

    float deltaLon = 2 * glm::pi<float>() / (lonIntervals < 5 ? 5 : lonIntervals);
    float deltaLat = glm::pi<float>() / (latIntervals < 5 ? latIntervals : latIntervals);

    float theta = 0.0f;

    TextureMeshIndexed mesh;
    mesh.texture = texture->getId();

    // do top vertex and indices
    mesh.vertices.emplace_back(top);
    uint32_t j=1;
    while (j<=lonIntervals) {
        mesh.indices.emplace_back(j+1);
        mesh.indices.emplace_back(j);
        mesh.indices.emplace_back(0);
        j++;
    }

    uint32_t lonOffset = 1;
    float vCoord=0;
    while (theta < glm::pi<float>()) {
        float phi = 0;
        float uCoord = 1;
        bool isBottom = glm::abs<float>(glm::pi<float>() - theta) < deltaLat;
        bool isTop = theta == deltaLat;

        if (isBottom) {
            mesh.vertices.emplace_back(bottom);
        } else {
            while (phi < 2 * glm::pi<float>() + deltaLon) {
                const glm::vec3 v = glm::vec3(
                    radius * glm::sin(theta) * glm::cos(phi),
                    radius * glm::cos(theta),
                    radius * glm::sin(theta) * glm::sin(phi)
                );
                const auto & vert =  TextureVertex { Vertex{ v, v * radDiv }, glm::vec2(uCoord, vCoord) };
                mesh.vertices.emplace_back(vert);

                phi += deltaLon;
                uCoord -=  1.0f / lonIntervals;
            }

            if (!isTop) lonOffset += lonIntervals+1;
        }

        if (isBottom) {
            j = lonOffset;
            while (j<=lonOffset+lonIntervals) {
                mesh.indices.emplace_back(static_cast<uint32_t>(mesh.vertices.size())-1);
                mesh.indices.emplace_back(j);
                mesh.indices.emplace_back(j+1);
                j++;
            }
        } else if (!isTop) {
            while (j<=lonOffset+lonIntervals) {
                mesh.indices.emplace_back(j);
                mesh.indices.emplace_back(j-lonIntervals-1);
                mesh.indices.emplace_back(j-lonIntervals);
                mesh.indices.emplace_back(j+1);
                mesh.indices.emplace_back(j);
                mesh.indices.emplace_back(j-lonIntervals);
                j++;
            }
        }

        theta += deltaLat;
        vCoord += 1.0 / latIntervals;
    }

    geom->meshes.emplace_back(mesh);

    return geom;
}

std::unique_ptr<ColorMeshGeometry> Helper::createBoxColorMeshGeometry(const float& width, const float& height, const float& depth, const glm::vec4& color)
{
    auto geom = std::make_unique<ColorMeshGeometry>();

    const auto & middle = glm::vec3 {width, height, depth} * .5f;
    geom->bbox.center = glm::vec3 {0.0f };
    geom->bbox.min = { -middle.x, -middle.y, -middle.z };
    geom->bbox.max = { middle.x, middle.y, middle.z };

    const float len = glm::sqrt(middle.x * middle.x + middle.y * middle.y + middle.z * middle.z);
    geom->bbox.radius = len;

    VertexMeshIndexed mesh;
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
        2, 4, 7,
        7, 3, 2,
        7, 4, 5,
        5, 6, 7,
        0, 1, 2,
        2, 3, 0,
        0, 6, 5,
        5, 1, 0,
        0, 3, 7,
        7, 6, 0,
        2, 5, 4,
        1, 5, 2
    };

    geom->meshes.emplace_back(mesh);

    return geom;
}

std::unique_ptr<TextureMeshGeometry> Helper::createBoxTextureMeshGeometry(const float& width, const float& height, const float& depth, const std::string & textureName, const glm::vec2 & middlePoint)
{
    const auto texture = GlobalTextureStore::INSTANCE()->getTextureByName(textureName);
    if (texture == nullptr) {
        logError("Please provide an existing Texture from the store for the Box Geometry");
        return nullptr;
    }

    if (!texture->isValid()) {
        logError("Could not load Texture: " + texture->getPath() + " for Box Geometry");
        return nullptr;
    }

    auto geom = std::make_unique<TextureMeshGeometry>();

    const auto & middle = glm::vec3 {width, height, depth} * .5f;
    geom->bbox.center = glm::vec3 {0.0f };
    geom->bbox.min = { -middle.x, -middle.y, -middle.z };
    geom->bbox.max = { middle.x, middle.y, middle.z };

    const float len = glm::sqrt(middle.x * middle.x + middle.y * middle.y + middle.z * middle.z);
    geom->bbox.radius = len;

    TextureMeshIndexed mesh;
    mesh.texture = texture->getId();

    mesh.vertices.emplace_back(TextureVertex { Vertex{ {-middle.x, middle.y, -middle.z}, glm::vec3 { -middle.x, middle.y, -middle.z  } / len }, glm::vec2(0.0f,middlePoint.y/2) }); //ok
    mesh.vertices.emplace_back(TextureVertex { Vertex{ {-middle.x, -middle.y, -middle.z }, glm::vec3 { -middle.x, -middle.y, -middle.z  } / len }, glm::vec2(middlePoint.x/2, middlePoint.y/2)}); //ok
    mesh.vertices.emplace_back(TextureVertex { Vertex{ {middle.x, middle.y, -middle.z }, glm::vec3 { middle.x,middle.y, -middle.z  } / len }, glm::vec2(0.0f,middlePoint.y)}); // 0k
    mesh.vertices.emplace_back(TextureVertex { Vertex{ {middle.x, -middle.y, -middle.z  }, glm::vec3 { middle.x, -middle.y, -middle.z  } / len }, glm::vec2(middlePoint.x/2, middlePoint.y)}); // ok

    mesh.vertices.emplace_back(TextureVertex { Vertex{ {-middle.x, -middle.y, middle.z  }, glm::vec3 { -middle.x, -middle.y, middle.z  } / len }, glm::vec2(middlePoint.x,middlePoint.y/2)});
    mesh.vertices.emplace_back(TextureVertex { Vertex{ {middle.x, -middle.y, middle.z  }, glm::vec3 { middle.x, -middle.y, middle.z  } / len }, glm::vec2(middlePoint.x,middlePoint.y)});
    mesh.vertices.emplace_back(TextureVertex { Vertex{ {-middle.x, middle.y, middle.z  }, glm::vec3 { -middle.x, middle.y, middle.z  } / len }, glm::vec2((middlePoint.x/2)*3, middlePoint.y/2)});
    mesh.vertices.emplace_back(TextureVertex { Vertex{ {middle.x, middle.y, middle.z  }, glm::vec3 { middle.x, middle.y, middle.z  } / len }, glm::vec2((middlePoint.x/2)*3, middlePoint.y)});

    mesh.vertices.emplace_back(TextureVertex { Vertex{ {-middle.x, middle.y, -middle.z}, glm::vec3 { -middle.x, middle.y, -middle.z  } / len }, glm::vec2(1.0f,middlePoint.y/2) }); // ok
    mesh.vertices.emplace_back(TextureVertex { Vertex{ {middle.x, middle.y, -middle.z }, glm::vec3 { middle.x,middle.y, -middle.z  } / len }, glm::vec2(1.0f,middlePoint.y)});  // ok

    mesh.vertices.emplace_back(TextureVertex { Vertex{ {-middle.x, middle.y, -middle.z}, glm::vec3 { -middle.x, middle.y, -middle.z  } / len }, glm::vec2(middlePoint.x/2,0.0f) });
    mesh.vertices.emplace_back(TextureVertex { Vertex{ {-middle.x, middle.y, middle.z  }, glm::vec3 { -middle.x, middle.y, middle.z  } / len }, glm::vec2(middlePoint.x, 0.0f)});

    mesh.vertices.emplace_back(TextureVertex { Vertex{ {middle.x, middle.y, -middle.z }, glm::vec3 { middle.x,middle.y, -middle.z  } / len }, glm::vec2(middlePoint.x/2,1.0f)});
    mesh.vertices.emplace_back(TextureVertex { Vertex{ {middle.x, middle.y, middle.z  }, glm::vec3 { middle.x, middle.y, middle.z  } / len }, glm::vec2(middlePoint.x, 1.0f)});

    mesh.indices = {
        0, 2, 1, // front
        1, 2, 3,
        4, 5, 6, // back
        5, 7, 6,
        6, 7, 8, //top
        7, 9 ,8,
        1, 3, 4, //bottom
        3, 5, 4,
        1, 11,10,// left
        1, 4, 11,
        3, 12, 5,//right
        5, 12, 13
    };

    geom->meshes.emplace_back(mesh);

    return geom;
}

std::unique_ptr<VertexMeshGeometry> Helper::getBboxesFromRenderables(const Renderable * source, const glm::vec3 & color)
{
    if (source == nullptr) return nullptr;

    auto lines = std::make_unique<VertexMeshGeometry>();

    const auto & bbox = source->getBoundingBox(true);
    const auto & l = Helper::getBboxWireframe(bbox);

    VertexMesh mesh;
    mesh.color = glm::vec4(color, 1);

    mesh.vertices = std::move(l);
    lines->meshes.emplace_back(mesh);

    lines->bbox = Helper::createBoundingBoxFromMinMax(bbox.min, bbox.max);

    return lines;
}

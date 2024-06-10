#ifndef SRC_INCLUDES_GEOMETRY_INCL_H_
#define SRC_INCLUDES_GEOMETRY_INCL_H_

#include "texture.h"

struct BoundingBox final {
    glm::vec3 min = glm::vec3(INF);
    glm::vec3 max = glm::vec3(NEG_INF);
    glm::vec3 center = glm::vec3(0);
    float radius = 0.0f;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct TextureVertex : Vertex {
    glm::vec2 uv;
};

struct Mesh {
    std::vector<Vertex> vertices;
};

struct TextureMesh {
    std::vector<TextureVertex> vertices;
};

struct VertexMesh : Mesh {
    glm::vec4 color;
};

struct VertexMeshIndexed : Mesh {
    std::vector<uint32_t> indices;
    glm::vec4 color;
};

struct TextureMeshIndexed : TextureMesh {
    std::vector<uint32_t> indices;
    uint32_t texture;
};

struct ModelMeshIndexed : TextureMesh {
    std::vector<uint32_t> indices;
    TextureInformation textures;
};

template<typename M>
struct MeshGeometry {
    std::vector<M> meshes;
    BoundingBox bbox;
};

using ColorMeshGeometry = MeshGeometry<VertexMeshIndexed>;
using VertexMeshGeometry = MeshGeometry<VertexMesh>;
using TextureMeshGeometry = MeshGeometry<TextureMeshIndexed>;
using ModelMeshGeometry = MeshGeometry<ModelMeshIndexed>;

#endif



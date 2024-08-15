#ifndef SRC_INCLUDES_GEOMETRY_INCL_H_
#define SRC_INCLUDES_GEOMETRY_INCL_H_

#include "texture.h"

struct TextureVertex : Vertex {
    glm::vec2 uv;
};

struct ModelVertex : TextureVertex {
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

struct TextureMesh {
    std::vector<TextureVertex> vertices;
};

struct ModelMesh {
    std::vector<ModelVertex> vertices;
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

struct MaterialInformation {
    glm::vec4 color = glm::vec4(1.0f);
    glm::vec3 specularColor = glm::vec3(1.0f);
    float shininess = 10.0f;
};

struct ModelMeshIndexed : ModelMesh {
    std::vector<uint32_t> indices;
    TextureInformation textures;
    MaterialInformation material;
};

template<typename M>
struct MeshGeometry {
    std::vector<M> meshes;
    BoundingSphere sphere;
};

using ColorMeshGeometry = MeshGeometry<VertexMeshIndexed>;
using VertexMeshGeometry = MeshGeometry<VertexMesh>;
using TextureMeshGeometry = MeshGeometry<TextureMeshIndexed>;
using ModelMeshGeometry = MeshGeometry<ModelMeshIndexed>;

#endif



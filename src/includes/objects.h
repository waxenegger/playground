#ifndef SRC_INCLUDES_OBJECTS_INCL_H_
#define SRC_INCLUDES_OBJECTS_INCL_H_

#include "geometry.h"

struct ColorMeshDrawCommand {
    uint32_t    indexCount;
    uint32_t    indexOffset;
    int32_t     vertexOffset;
    uint32_t    firstInstance;
    uint32_t    meshInstance;
};

struct VertexMeshDrawCommand {
    uint32_t    vertexCount;
    uint32_t     vertexOffset;
    uint32_t    firstInstance;
    uint32_t    meshInstance;
};

struct ColorMeshIndirectDrawCommand {
    VkDrawIndexedIndirectCommand indirectDrawCommand;
    uint32_t meshInstance;
};

struct VertexMeshIndirectDrawCommand {
    VkDrawIndirectCommand indirectDrawCommand;
    uint32_t meshInstance;
};

struct ColorMeshInstanceData final {
    glm::mat4 matrix {1.0f};
    glm::vec3 center = glm::vec3{0.0f};
    float radius = 0;
};

struct ColorMeshData final {
    glm::vec4 color {1.0f};
};

struct TextureMeshData final {
    uint32_t texture = 0;
};

struct ModelMeshData final {
    MaterialInformation material;
    TextureInformation texture;
};

struct ColorMeshPushConstants final {
    glm::mat4 matrix {1.0f};
    glm::vec4 color {1.0f};
};

struct TextureMeshPushConstants final {
    glm::mat4 matrix {1.0f};
    uint32_t texture = 0;
};

struct ModelMeshPushConstants {
    glm::mat4 matrix {1.0f};
    MaterialInformation material;
    TextureInformation texture;
};

class Renderable {
    protected:
        std::string id;

        BoundingBox bbox;
        BoundingSphere sphere;

        glm::mat4 matrix { 1.0f };
        glm::vec3 position {0.0f};
        glm::vec3 rotation { 0.0f };
        float scaling = 1.0f;

        Renderable(const std::string id);
        void updateMatrix();
        void updateBbox(const glm::mat4 & invMatrix = glm::mat4(0.0f), const bool forceRecalculation = false);

    private:
        bool dirty = false;
        bool registered = false;
        bool frustumCulled = false;

    public:
        Renderable(const Renderable&) = delete;
        Renderable& operator=(const Renderable &) = delete;
        Renderable(Renderable &&) = delete;

        bool shouldBeRendered() const;
        void setDirty(const bool & dirty);
        bool isDirty() const;
        void flagAsRegistered();
        bool hasBeenRegistered();
        void performFrustumCulling(const std::array<glm::vec4, 6> & frustumPlanes);

        void setPosition(const glm::vec3 position);
        void setScaling(const float factor);
        void setRotation(glm::vec3 rotation);

        glm::vec3 getUnitDirectionVector(const float leftRightAngle = 0.0f);
        void move(const float delta = 0.0f, const Direction & direction = { false, false, true, false });
        void rotate(int xAxis = 0, int yAxis = 0, int zAxis = 0);

        const glm::vec3 getPosition() const;
        const glm::vec3 getRotation() const;
        const float getScaling() const;
        const glm::mat4 getMatrix() const;

        const BoundingBox getBoundingBox() const;
        void  setBoundingBox(const BoundingBox bbox);
        const BoundingSphere getBoundingSphere() const;

        const std::string getId() const;

        virtual ~Renderable();
};

template<typename M, typename G>
class MeshRenderable : public Renderable {
    protected:
        std::vector<M> meshes;
    public:
        MeshRenderable(const MeshRenderable&) = delete;
        MeshRenderable& operator=(const MeshRenderable &) = delete;
        MeshRenderable(MeshRenderable &&) = delete;
        MeshRenderable(const std::string name) : Renderable(name) {}
        MeshRenderable(const std::string name, const std::unique_ptr<G> & geometry) : MeshRenderable(name) {
            this->meshes = std::move(geometry->meshes);
            this->bbox = geometry->bbox;
            this->sphere = this->bbox.getBoundingSphere();
        };

        void setMeshes(const std::vector<M> & meshes) { this->meshes = std::move(meshes);};
        const std::vector<M> & getMeshes() const { return this->meshes;};
        void setBBox(const BoundingBox & bbox) {
            this->bbox = std::move(bbox);
            this->sphere = this->bbox.getBoundingSphere();
        };
};

using ColorMeshRenderable = MeshRenderable<VertexMeshIndexed, ColorMeshGeometry>;
using VertexMeshRenderable = MeshRenderable<VertexMesh, VertexMeshGeometry>;
using TextureMeshRenderable = MeshRenderable<TextureMeshIndexed, TextureMeshGeometry>;
using ModelMeshRenderable = MeshRenderable<ModelMeshIndexed, ModelMeshGeometry>;

using GlobalRenderableStore = GlobalObjectStore<Renderable>;

struct ShaderConfig {
    std::string file;
    VkShaderStageFlagBits shaderType = VK_SHADER_STAGE_VERTEX_BIT;
};

struct PipelineConfig {
    public:
        virtual ~PipelineConfig() = default;
        std::vector<ShaderConfig> shaders;
};

struct ImGUIPipelineConfig : PipelineConfig {};

class Pipeline;
struct GraphicsPipelineConfig : PipelineConfig {
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool enableColorBlend = true;
    bool enableDepth = true;

    VkDeviceSize reservedVertexSpace = 500 * MEGA_BYTE;
    bool useDeviceLocalForVertexSpace = false;
    VkDeviceSize reservedIndexSpace = 500 * MEGA_BYTE;
    bool useDeviceLocalForIndexSpace = false;
    VkDeviceSize reservedInstanceDataSpace = 50 * MEGA_BYTE;
    VkDeviceSize reservedMeshDataSpace = 50 * MEGA_BYTE;
    VkDeviceSize reservedAnimationDataSpace = 50 * MEGA_BYTE;
};

struct ColorMeshPipelineConfig : GraphicsPipelineConfig {
    std::vector<ColorMeshRenderable *> objectsToBeRendered;
    int indirectBufferIndex = -1;

    ColorMeshPipelineConfig(const bool useGpuCulling) {
        this->shaders = {
            { "color_meshes" + std::string(useGpuCulling ? "_gpu" : "") + ".vert.spv" , VK_SHADER_STAGE_VERTEX_BIT },
            { "color_meshes" + std::string(useGpuCulling ? "_gpu" : "") + ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};

struct VertexMeshPipelineConfig : GraphicsPipelineConfig {
    std::vector<VertexMeshRenderable *> objectsToBeRendered;
    int indirectBufferIndex = -1;

    VertexMeshPipelineConfig(const bool useGpuCulling) {
        if (useGpuCulling) {
            this->shaders = {
                { "vertex_meshes_gpu.vert.spv" , VK_SHADER_STAGE_VERTEX_BIT },
                { "color_meshes_gpu.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
            };
        } else {
            this->shaders = {
                { "color_meshes" + std::string(useGpuCulling ? "_gpu" : "") + ".vert.spv" , VK_SHADER_STAGE_VERTEX_BIT },
                { "color_meshes" + std::string(useGpuCulling ? "_gpu" : "") + ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
            };
        }
    };
};

struct TextureMeshPipelineConfig : GraphicsPipelineConfig {
    std::vector<TextureMeshRenderable *> objectsToBeRendered;
    int indirectBufferIndex = -1;

    TextureMeshPipelineConfig(const bool useGpuCulling) {
        this->shaders = {
            { "texture_meshes" + std::string(useGpuCulling ? "_gpu" : "") + ".vert.spv" , VK_SHADER_STAGE_VERTEX_BIT },
            { "texture_meshes" + std::string(useGpuCulling ? "_gpu" : "") + ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};

struct ModelMeshPipelineConfig : GraphicsPipelineConfig {
    std::vector<ModelMeshRenderable *> objectsToBeRendered;
    int indirectBufferIndex = -1;

    ModelMeshPipelineConfig(const bool useGpuCulling) {
        this->shaders = {
            { "model_meshes" + std::string(useGpuCulling ? "_gpu" : "") + ".vert.spv" , VK_SHADER_STAGE_VERTEX_BIT },
            { "model_meshes" + std::string(useGpuCulling ? "_gpu" : "") + ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};

class AnimatedModelMeshRenderable;
struct AnimatedModelMeshPipelineConfig : GraphicsPipelineConfig {
    std::vector<AnimatedModelMeshRenderable *> objectsToBeRendered;
    int indirectBufferIndex = -1;

    AnimatedModelMeshPipelineConfig(const bool useGpuCulling) {
        this->shaders = {
            { "animated_model_meshes" + std::string(useGpuCulling ? "_gpu" : "") + ".vert.spv" , VK_SHADER_STAGE_VERTEX_BIT },
            { "model_meshes" + std::string(useGpuCulling ? "_gpu" : "") + ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};

struct SkyboxPipelineConfig : GraphicsPipelineConfig {
    std::array<std::string, 6> skyboxImages = { "front.tga", "back.tga", "top.tga", "bottom.tga", "right.tga" , "left.tga" };
    //std::array<std::string, 6> skyboxImages = { "right.png", "left.png", "top.png", "bottom.png", "front.png", "back.png" };

    SkyboxPipelineConfig() {
        this->shaders = {
            { "skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT },
            { "skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};

#endif




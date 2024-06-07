#ifndef SRC_INCLUDES_OBJECTS_INCL_H_
#define SRC_INCLUDES_OBJECTS_INCL_H_

#include "shared.h"

class Renderable {
    protected:
        BoundingBox bbox;

        Renderable();
        void updateMatrix();
        void updateBbox(const glm::mat4 & invMatrix = glm::mat4(0.0f));

        std::vector<Renderable *> debugRenderable;

    private:
        bool dirty = false;
        bool registered = false;

        glm::mat4 matrix { 1.0f };
        glm::vec3 position {0.0f};
        glm::vec3 rotation { 0.0f };
        float scaling = 1.0f;

    public:

        Renderable(const Renderable&) = delete;
        Renderable& operator=(const Renderable &) = delete;
        Renderable(Renderable &&) = delete;

        bool shouldBeRendered(const std::array<glm::vec4, 6> & frustumPlanes) const;
        void setDirty(const bool & dirty);
        bool isDirty() const;
        void flagAsRegistered();
        bool hasBeenRegistered();
        bool isInFrustum(const std::array<glm::vec4, 6> & frustumPlanes) const;

        void setPosition(const glm::vec3 position);
        void setScaling(const float factor);
        void setRotation(glm::vec3 rotation);

        glm::vec3 getFront(const float leftRightAngle = 0.0f);
        void move(const float delta, const Direction & direction = { false, false, true, false });
        void rotate(int xAxis = 0, int yAxis = 0, int zAxis = 0);

        const glm::vec3 getPosition() const;
        const glm::vec3 getRotation() const;
        const float getScaling() const;
        const glm::mat4 getMatrix() const;
        void addDebugRenderable(Renderable * renderable);

        const BoundingBox getBoundingBox(const bool & withoutTransformations = false) const;

        virtual ~Renderable();
};

template<typename M, typename G>
class MeshRenderable : public Renderable {
    private:
        std::vector<M> meshes;
    public:
        MeshRenderable(const MeshRenderable&) = delete;
        MeshRenderable& operator=(const MeshRenderable &) = delete;
        MeshRenderable(MeshRenderable &&) = delete;
        MeshRenderable() : Renderable() {}
        MeshRenderable(const std::unique_ptr<G> & geometry) {
            this->meshes = std::move(geometry->meshes);
            this->bbox = geometry->bbox;
        };
        void setMeshes(const std::vector<M> & meshes) {
            this->meshes = std::move(meshes);
        };
        const std::vector<M> & getMeshes() const {
            return this->meshes;
        };
};

using ColorMeshRenderable = MeshRenderable<VertexMeshIndexed, ColorMeshGeometry>;
using VertexMeshRenderable = MeshRenderable<VertexMesh, VertexMeshGeometry>;
using TemplateMeshRenderable = MeshRenderable<TextureMeshIndexed, VertexMeshGeometry>;
using MeshRenderableVariant = std::variant<std::nullptr_t, ColorMeshRenderable *, VertexMeshRenderable *>;

class GlobalRenderableStore final {
    private:
        static GlobalRenderableStore * instance;
        GlobalRenderableStore();

        std::vector<std::unique_ptr<Renderable>> objects;

    public:
        GlobalRenderableStore& operator=(const GlobalRenderableStore &) = delete;
        GlobalRenderableStore(GlobalRenderableStore &&) = delete;
        GlobalRenderableStore & operator=(GlobalRenderableStore) = delete;

        static GlobalRenderableStore * INSTANCE();

        template<typename R>
        R * registerRenderable(std::unique_ptr<R> & renderableObject) {
            renderableObject->flagAsRegistered();
            this->objects.emplace_back(std::move(renderableObject));

            return static_cast<R *>(this->objects[this->objects.empty() ? 1 : this->objects.size()-1].get());
        };

        const std::vector<std::unique_ptr<Renderable>> & getRenderables() const;

        ~GlobalRenderableStore();
};

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

    Pipeline * pipelineToDebug = nullptr;
    bool showNormals = false;
    bool showBboxes = false;

    VkDeviceSize reservedVertexSpace = 500 * MEGA_BYTE;
    bool useDeviceLocalForVertexSpace = false;
    VkDeviceSize reservedIndexSpace = 500 * MEGA_BYTE;
    bool useDeviceLocalForIndexSpace = false;
    VkDeviceSize reservedInstanceDataSpace = 50 * MEGA_BYTE;
    VkDeviceSize reservedMeshDataSpace = 50 * MEGA_BYTE;
};

struct ColorMeshPipelineConfig : GraphicsPipelineConfig {
    std::vector<ColorMeshRenderable *> objectsToBeRendered;
    int indirectBufferIndex = -1;

    ColorMeshPipelineConfig() {
        this->shaders = {
            { "color_meshes" + std::string(USE_GPU_CULLING ? "_gpu" : "") + ".vert.spv" , VK_SHADER_STAGE_VERTEX_BIT },
            { "color_meshes" + std::string(USE_GPU_CULLING ? "_gpu" : "") + ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};

struct VertexMeshPipelineConfig : GraphicsPipelineConfig {
    std::vector<VertexMeshRenderable *> objectsToBeRendered;
    int indirectBufferIndex = -1;

    VertexMeshPipelineConfig() {
        if (USE_GPU_CULLING) {
            this->shaders = {
                { "vertex_meshes_gpu.vert.spv" , VK_SHADER_STAGE_VERTEX_BIT },
                { "color_meshes_gpu.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
            };
        } else {
            this->shaders = {
                { "color_meshes" + std::string(USE_GPU_CULLING ? "_gpu" : "") + ".vert.spv" , VK_SHADER_STAGE_VERTEX_BIT },
                { "color_meshes" + std::string(USE_GPU_CULLING ? "_gpu" : "") + ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
            };
        }
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




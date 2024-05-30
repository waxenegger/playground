#ifndef SRC_INCLUDES_OBJECTS_INCL_H_
#define SRC_INCLUDES_OBJECTS_INCL_H_

#include "shared.h"

class Renderable {
    protected:
        BoundingBox bbox;

        Renderable();
        void updateMatrix();
        void updateBbox(const glm::mat4 & invMatrix = glm::mat4(0.0f));

private:
        bool inactive = false;
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
        void setInactive(const bool & inactive);
        void setDirty(const bool & dirty);
        void flagAsRegistered();
        bool hasBeenRegistered();
        bool isInFrustum(const std::array<glm::vec4, 6> & frustumPlanes) const;

        void setPosition(const glm::vec3 & position);
        void setScaling(const float & factor);
        void setRotation(glm::vec3 & rotation);

        glm::vec3 getFront(const float leftRightAngle = 0.0f);
        void move(const float delta, const Direction & direction = { false, false, true, false });
        void rotate(int xAxis = 0, int yAxis = 0, int zAxis = 0);

        const glm::vec3 getPosition() const;
        const float getScaling() const;
        const glm::mat4 getMatrix() const;

        const BoundingBox & getBoundingBox() const;

        virtual ~Renderable();
};

class ColorMeshRenderable : public Renderable {
    private:
        std::vector<VertexMesh *> meshes;

    public:
        ColorMeshRenderable(const ColorMeshRenderable&) = delete;
        ColorMeshRenderable& operator=(const ColorMeshRenderable &) = delete;
        ColorMeshRenderable(ColorMeshRenderable &&) = delete;
        ColorMeshRenderable();
        ColorMeshRenderable(const std::unique_ptr<ColorMeshGeometry> & geometry);

        void setMeshes(const std::vector<VertexMesh *> & meshes);
        const std::vector<VertexMesh *> & getMeshes() const;

        ~ColorMeshRenderable() noexcept;
};

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
        template <typename T>
        T getRenderableByIndex(const uint32_t & index) {
            if (index >= this->objects.size()) return nullptr;

            T ret;
            try {
                ret = dynamic_cast<T>(this->objects[index].get());
            } catch(std::bad_cast ex) {
                return nullptr;
            }

            return ret;
        }

        void registerRenderable(Renderable * renderableObject);

        const std::vector<std::unique_ptr<Renderable>> & getRenderables() const;

        ~GlobalRenderableStore();
};

enum PipelineConfigType {
    Unknown = 0, GenericGraphics, Compute, ColorMesh, ImGUI, SkyBox
};

struct ShaderConfig {
    std::string file;
    VkShaderStageFlagBits shaderType = VK_SHADER_STAGE_VERTEX_BIT;
};

struct PipelineConfig {
    protected:
        enum PipelineConfigType type;
    public:
        virtual ~PipelineConfig() = default;
        std::vector<ShaderConfig> shaders;
        PipelineConfigType getType() const { return this->type; };
};

struct ImGUIPipelineConfig : PipelineConfig
{
    ImGUIPipelineConfig() { this->type = ImGUI; };
};

struct ComputePipelineConfig : PipelineConfig {
    ComputePipelineConfig() {
        this->type = Compute;
        this->shaders = { {"cull.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT} };
    };

    VkDeviceSize reservedComputeSpace = 0;
};

struct GenericGraphicsPipelineConfig : PipelineConfig {
    GenericGraphicsPipelineConfig() { this->type = GenericGraphics; };

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool enableColorBlend = true;
    bool enableDepth = true;

    VkDeviceSize reservedVertexSpace = 0;
    VkDeviceSize reservedIndexSpace = 0;
};

struct ColorMeshPipelineConfig : GenericGraphicsPipelineConfig {
    std::vector<ColorMeshRenderable *> objectsToBeRendered;

    ColorMeshPipelineConfig() {
        this->type = ColorMesh;
        this->shaders = {
            { "color_meshes.vert.spv", VK_SHADER_STAGE_VERTEX_BIT },
            { "color_meshes.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};

struct SkyboxPipelineConfig : GenericGraphicsPipelineConfig {
    std::array<std::string, 6> skyboxImages = { "front.tga", "back.tga", "top.tga", "bottom.tga", "right.tga" , "left.tga" };
    //std::array<std::string, 6> skyboxImages = { "right.png", "left.png", "top.png", "bottom.png", "front.png", "back.png" };

    SkyboxPipelineConfig() {
        this->type = SkyBox;

        this->shaders = {
            { "skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT },
            { "skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};



#endif




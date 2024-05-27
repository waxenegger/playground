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

class ColorVerticesRenderable : public Renderable {
    private:
        std::vector<ColorVertex> vertices;
        std::vector<uint32_t> indices;

    public:
        ColorVerticesRenderable(const ColorVerticesRenderable&) = delete;
        ColorVerticesRenderable& operator=(const ColorVerticesRenderable &) = delete;
        ColorVerticesRenderable(ColorVerticesRenderable &&) = delete;
        ColorVerticesRenderable();
        ColorVerticesRenderable(const ColorVertexGeometry & geometry);

        void setVertices(const std::vector<ColorVertex> & vertices);
        const std::vector<ColorVertex> & getVertices() const;

        void setIndices(const std::vector<uint32_t> & indices);
        const std::vector<uint32_t> & getIndices() const;
};

class ColorMeshRenderable : public Renderable {
    private:
        std::vector<VertexMesh> meshes;

    public:
        ColorMeshRenderable(const ColorMeshRenderable&) = delete;
        ColorMeshRenderable& operator=(const ColorMeshRenderable &) = delete;
        ColorMeshRenderable(ColorMeshRenderable &&) = delete;
        ColorMeshRenderable();
        ColorMeshRenderable(const ColorMeshGeometry & geometry);

        void setMeshes(const std::vector<Vertex> & meshes);
        const std::vector<VertexMesh> & getMeshes() const;
};

class StaticColorVerticesRenderable : public ColorVerticesRenderable {
    public:
        StaticColorVerticesRenderable(const StaticColorVerticesRenderable&) = delete;
        StaticColorVerticesRenderable& operator=(const StaticColorVerticesRenderable &) = delete;
        StaticColorVerticesRenderable(StaticColorVerticesRenderable &&) = delete;
        StaticColorVerticesRenderable();
        StaticColorVerticesRenderable(const ColorVertexGeometry & geometry);

        void setPosition(const glm::vec3 & position);
        void setScaling(const float & factor);
        void setRotation(glm::vec3 & rotation);
        void move(const float delta, const Direction & direction = { false, false, true, false });
        void rotate(int xAxis = 0, int yAxis = 0, int zAxis = 0);
};

class DynamicColorVerticesRenderable : public ColorVerticesRenderable {
    public:
        DynamicColorVerticesRenderable(const DynamicColorVerticesRenderable&) = delete;
        DynamicColorVerticesRenderable& operator=(const DynamicColorVerticesRenderable &) = delete;
        DynamicColorVerticesRenderable(DynamicColorVerticesRenderable &&) = delete;
        DynamicColorVerticesRenderable();
        DynamicColorVerticesRenderable(const ColorVertexGeometry & geometry);
};

class StaticColorMeshRenderable : public ColorMeshRenderable {
    public:
        StaticColorMeshRenderable(const StaticColorMeshRenderable&) = delete;
        StaticColorMeshRenderable& operator=(const StaticColorMeshRenderable &) = delete;
        StaticColorMeshRenderable(StaticColorMeshRenderable &&) = delete;
        StaticColorMeshRenderable();
        StaticColorMeshRenderable(const ColorMeshGeometry & geometry);

        void setPosition(const glm::vec3 & position);
        void setScaling(const float & factor);
        void setRotation(glm::vec3 & rotation);
        void move(const float delta, const Direction & direction = { false, false, true, false });
        void rotate(int xAxis = 0, int yAxis = 0, int zAxis = 0);
};

class DynamicColorMeshRenderable : public ColorMeshRenderable {
    public:
        DynamicColorMeshRenderable(const DynamicColorMeshRenderable&) = delete;
        DynamicColorMeshRenderable& operator=(const DynamicColorMeshRenderable &) = delete;
        DynamicColorMeshRenderable(DynamicColorMeshRenderable &&) = delete;
        DynamicColorMeshRenderable();
        DynamicColorMeshRenderable(const ColorMeshGeometry & geometry);
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

        ~GlobalRenderableStore();
};

enum PipelineConfigType {
    Unknown = 0, GenericGraphics,
    GenericObjectsColorVertex, StaticObjectsColorVertex, DynamicObjectsColorVertex,
    GenericObjectsColorMesh, StaticObjectsColorMesh, DynamicObjectsColorMesh,
    ImGUI, SkyBox
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

struct GenericGraphicsPipelineConfig : PipelineConfig {
    GenericGraphicsPipelineConfig() { this->type = GenericGraphics; };

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool enableColorBlend = true;
    bool enableDepth = true;

    VkDeviceSize reservedVertexSpace = 0;
    VkDeviceSize reservedIndexSpace = 0;
};

struct ColorVertexPipelineConfig : GenericGraphicsPipelineConfig {
    std::vector<ColorVerticesRenderable *> objectsToBeRendered;

    ColorVertexPipelineConfig() { this->type = GenericObjectsColorVertex;};
};

struct StaticObjectsColorVertexPipelineConfig : ColorVertexPipelineConfig {
    std::vector<StaticColorVerticesRenderable *> objectsToBeRendered;

    StaticObjectsColorVertexPipelineConfig() {
        this->type = StaticObjectsColorVertex;
        this->shaders = {
            { "static_color_vertices_minimal.vert.spv", VK_SHADER_STAGE_VERTEX_BIT },
            { "static_color_vertices_minimal.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};

struct DynamicObjectsColorVertexPipelineConfig : ColorVertexPipelineConfig {
    std::vector<DynamicColorVerticesRenderable *> objectsToBeRendered;

    DynamicObjectsColorVertexPipelineConfig() {
        this->type = DynamicObjectsColorVertex;
        this->shaders = {
            { "dynamic_color_vertices_minimal.vert.spv", VK_SHADER_STAGE_VERTEX_BIT },
            { "dynamic_color_vertices_minimal.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};

struct ColorMeshPipelineConfig : GenericGraphicsPipelineConfig {
    std::vector<ColorMeshRenderable *> objectsToBeRendered;

    ColorMeshPipelineConfig() { this->type = GenericObjectsColorMesh;};
};

struct StaticObjectsColorMeshPipelineConfig : ColorMeshPipelineConfig {
    std::vector<StaticColorMeshRenderable *> objectsToBeRendered;

    StaticObjectsColorMeshPipelineConfig() {
        this->type = StaticObjectsColorMesh;
        this->shaders = {
            { "static_color_mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT },
            { "static_color_mesh.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
        };
    };
};

struct DynamicObjectsColorMeshPipelineConfig : ColorMeshPipelineConfig {
    std::vector<DynamicColorMeshRenderable *> objectsToBeRendered;

    DynamicObjectsColorMeshPipelineConfig() {
        this->type = DynamicObjectsColorMesh;
        this->shaders = {
            { "dynamic_color_mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT },
            { "dynamic_color_mesh.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
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




#ifndef SRC_INCLUDES_ENGINE_INCL_H_
#define SRC_INCLUDES_ENGINE_INCL_H_

#include "graphics.h"

class Renderer;

class Shader final {
    private:
        std::string filename;
        VkShaderStageFlagBits shaderType;
        VkShaderModule shaderModule = nullptr;
        VkDevice device;

        bool readFile(const std::string & filename, std::vector<char> & buffer);
        VkShaderModule createShaderModule(const std::vector<char> & code);
    public:
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader &) = delete;
        Shader(Shader &&) = delete;
        Shader & operator=(Shader) = delete;

        VkShaderStageFlagBits getShaderType() const;
        VkShaderModule getShaderModule() const;
        std::string getFileName() const;

        Shader(const VkDevice device, const std::string & filename, const VkShaderStageFlagBits shaderType);
        ~Shader();

        bool isValid() const;
};

struct ShaderConfig {
    std::string file;
    VkShaderStageFlagBits shaderType = VK_SHADER_STAGE_VERTEX_BIT;
};

enum PipelineConfigType {
    Unknown = 0, GenericGraphics, StaticColor, ImGUI, SkyBox
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

    std::vector<ColorVertex> colorVertices;
    std::vector<uint32_t> indices;
};

struct StaticColorVertexPipelineConfig : GenericGraphicsPipelineConfig {
    StaticColorVertexPipelineConfig() {
        this->type = StaticColor;
        this->shaders = {
            { "static_color_vertices_minimal.vert.spv", VK_SHADER_STAGE_VERTEX_BIT },
            { "static_color_vertices_minimal.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT }
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

class Pipeline {
    protected:
        std::string name;
        std::map<std::string, const Shader *> shaders;
        bool enabled = true;

        Renderer * renderer = nullptr;
        VkPipeline pipeline = nullptr;

        DescriptorPool descriptorPool;
        Descriptors descriptors;
        VkPipelineLayout layout = nullptr;

        uint8_t getNumberOfValidShaders() const;
    public:
        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline &) = delete;
        Pipeline(Pipeline &&) = delete;
        Pipeline(const std::string name, Renderer * renderer);

        std::string getName() const;
        void setName(const std::string name);


        bool addShader(const std::string & file, const VkShaderStageFlagBits & shaderType);
        std::vector<VkPipelineShaderStageCreateInfo> getShaderStageCreateInfos();

        virtual bool isReady() const = 0;
        virtual bool canRender() const = 0;
        virtual bool initPipeline(const PipelineConfig & config) = 0;
        virtual bool createPipeline() = 0;
        virtual void destroyPipeline();

        bool hasPipeline() const;
        bool isEnabled();
        void setEnabled(const bool flag);

        virtual ~Pipeline();
};

class Renderer final {
    private:
        const GraphicsContext * graphicsContext = nullptr;
        const VkPhysicalDevice physicalDevice = nullptr;
        VkDevice logicalDevice = nullptr;

        std::map<std::string, uint64_t> deviceProperties;
        bool memoryBudgetExtensionSupported = false;

        CommandPool graphicsCommandPool;

        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t imageCount = DEFAULT_BUFFERING;

        int graphicsQueueIndex = -1;
        VkQueue graphicsQueue = nullptr;

        VkClearColorValue clearValue = BLACK;

        VkPhysicalDeviceMemoryProperties memoryProperties;

        std::vector<Buffer> uniformBuffer;

        std::vector<Pipeline *> pipelines;

        std::vector<float> deltaTimes;
        float lastDeltaTime = DELTA_TIME_60FPS;
        uint64_t accumulatedDeltaTime = 0;

        std::chrono::high_resolution_clock::time_point lastFrameRateUpdate;
        uint16_t frameRate = FRAME_RATE_60;
        size_t currentFrame = 0;

        bool paused = true;
        bool requiresRenderUpdate = false;
        bool showWireFrame = false;
        bool maximized = false;
        bool fullScreen = false;

        VkRenderPass renderPass = nullptr;
        VkExtent2D swapChainExtent;

        VkSwapchainKHR swapChain = nullptr;
        std::vector<Image> swapChainImages;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::vector<Image> depthImages;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        bool createRenderPass();
        bool createSwapChain();
        bool createFramebuffers();
        bool createDepthResources();

        bool createCommandPools();
        bool createSyncObjects();

        bool createCommandBuffers();
        void destroyCommandBuffer(VkCommandBuffer commandBuffer, const bool resetOnly = false);
        VkCommandBuffer createCommandBuffer(const uint16_t commandBufferIndex, const uint16_t imageIndex, const bool useSecondaryCommandBuffers = false);

        void update();
        void renderFrame();

        bool createUniformBuffers();
        void updateUniformBuffers(int index, uint32_t componentsDrawCount = 0);

        void destroySwapChainObjects();
        void destroyRendererObjects();

         void setPhysicalDeviceProperties();
    public:

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer &) = delete;
        Renderer(Renderer &&) = delete;
        Renderer & operator=(Renderer) = delete;
        Renderer(const GraphicsContext * graphicsContext, const VkPhysicalDevice & physicalDevice, const int & graphicsQueueIndex);

        bool addPipeline(std::unique_ptr<Pipeline> & pipeline, const int index = -1);
        void enablePipeline(const std::string name, const bool flag = true);
        void removePipeline(const std::string name);

        bool isReady() const;
        bool hasAtLeastOneActivePipeline() const;
        virtual bool canRender() const;

        VkDevice getLogicalDevice() const;
        VkPhysicalDevice getPhysicalDevice() const;
        uint32_t getImageCount() const;

        float getDeltaTime() const;
        uint16_t getFrameRate() const;
        void addDeltaTime(const std::chrono::high_resolution_clock::time_point now, const float deltaTime);
        uint64_t getAccumulatedDeltaTime();

        void initRenderer();
        bool recreateRenderer();

        void forceRenderUpdate();
        bool doesShowWireFrame() const;
        void setShowWireFrame(bool showWireFrame);

        VkRenderPass getRenderPass() const;
        VkExtent2D getSwapChainExtent() const;

        const CommandPool & getGraphicsCommandPool() const;

        VkQueue getGraphicsQueue() const;
        uint32_t getGraphicsQueueIndex() const;

        void setClearValue(const VkClearColorValue & clearColorValue);

        const VkPhysicalDeviceMemoryProperties & getMemoryProperties() const;
        uint64_t getPhysicalDeviceProperty(const std::string prop) const;
        VkDeviceSize getAvailableDeviceMemory() const;
        void trackDeviceLocalMemory(const VkDeviceSize & delta,  const bool & isFree = false);

        Pipeline * getPipeline(const std::string name);

        const Buffer & getUniformBuffer(int index) const;

        const GraphicsContext * getGraphicsContext() const;

        void render();

        void pause();
        bool isPaused();
        void resume();

        bool isMaximized();
        bool isFullScreen();

        ~Renderer();
};

class PipelineFactory final {
    private:
        Renderer * renderer = nullptr;
    public:
        PipelineFactory(const PipelineFactory&) = delete;
        PipelineFactory& operator=(const PipelineFactory &) = delete;
        PipelineFactory(PipelineFactory &&) = delete;
        PipelineFactory(Renderer * renderer);

        Pipeline * create(const std::string & name, const PipelineConfig & pipelineConfig);
        Pipeline * create(const std::string & name, const StaticColorVertexPipelineConfig & staticColorVertexConfig);
        Pipeline * create(const std::string & name, const SkyboxPipelineConfig & skyboxPipelineConfig);
        Pipeline * create(const std::string & name, const GenericGraphicsPipelineConfig & genericGraphicsPipelineConfig);
        Pipeline * create(const std::string & name, const ImGUIPipelineConfig & imGuiPipelineConfig);
};

class Engine final {
    private:
        static std::filesystem::path base;
        GraphicsContext * graphics = new GraphicsContext();
        Camera * camera = Camera::INSTANCE();
        Renderer * renderer = nullptr;
        PipelineFactory * pipelineFactory = nullptr;

        bool quit = false;

        void createRenderer();

        void inputLoopSdl();
        void render(const std::chrono::high_resolution_clock::time_point & frameStart);
    public:
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine &) = delete;
        Engine(Engine &&) = delete;
        Engine & operator=(Engine) = delete;

        bool isGraphicsActive();
        bool isReady();

        Pipeline * getPipeline(const std::string name);
        bool addPipeline(const std::string name, const PipelineConfig & config, const int index = -1);
        void removePipeline(const std::string name);
        void enablePipeline(const std::string name, const bool flag = true);
        void setBackDrop(const VkClearColorValue & clearColor);

        Renderer * getRenderer() const;

        void init();
        void loop();

        Engine(const std::string & appName, const std::string root = "", const uint32_t version = VULKAN_VERSION);
        ~Engine();

        static std::filesystem::path getAppPath(APP_PATHS appPath);

        Camera * getCamera();
};

class GraphicsPipeline : public Pipeline {
    protected:
        VkPushConstantRange pushConstantRange {};
        VkSampler textureSampler = nullptr;

        Buffer vertexBuffer;
        Buffer indexBuffer;
        Buffer ssboMeshBuffer;
        Buffer ssboInstanceBuffer;
        Buffer animationMatrixBuffer;
    public:
        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline &) = delete;
        GraphicsPipeline(GraphicsPipeline &&) = delete;

        bool createGraphicsPipelineCommon(const bool doColorBlend = true, const bool hasDepth = true, const bool cullBack = true, const VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        virtual bool initPipeline(const PipelineConfig & config) = 0;
        virtual bool createPipeline() = 0;

        virtual void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex) = 0;
        virtual void update() = 0;

        bool isReady() const;
        bool canRender() const;

        void correctViewPortCoordinates(const VkCommandBuffer & commandBuffer);

        GraphicsPipeline(const std::string name, Renderer * renderer);
        ~GraphicsPipeline();
};

class ImGuiPipeline : public GraphicsPipeline {
    private:
        ImGUIPipelineConfig config;

    public:
        ImGuiPipeline(const std::string name, Renderer * renderer);
        ImGuiPipeline & operator=(ImGuiPipeline) = delete;
        ImGuiPipeline(const ImGuiPipeline&) = delete;
        ImGuiPipeline(ImGuiPipeline &&) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();
        bool canRender() const;

        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update();

        ~ImGuiPipeline();
};

class StaticObjectsColorVertexPipeline : public GraphicsPipeline {
    private:
        StaticColorVertexPipelineConfig config;

        std::vector<ColorVertex> vertices;
        std::vector<uint32_t> indexes;

        bool createBuffers();

        bool createDescriptorPool();
        bool createDescriptors();

    public:
        StaticObjectsColorVertexPipeline(const std::string name, Renderer * renderer);
        StaticObjectsColorVertexPipeline & operator=(StaticObjectsColorVertexPipeline) = delete;
        StaticObjectsColorVertexPipeline(const StaticObjectsColorVertexPipeline&) = delete;
        StaticObjectsColorVertexPipeline(StaticObjectsColorVertexPipeline &&) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();

        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update();
};

class SkyboxPipeline : public GraphicsPipeline {
    private:
        SkyboxPipelineConfig config;

        const std::vector<glm::vec3> SKYBOX_VERTICES = {
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, 1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f),
            glm::vec3(-1.0f, 1.0f, -1.0f)
        };

        const std::vector<uint32_t> SKYBOX_INDEXES = {
            7, 4, 2, 2, 3, 7,
            5, 4, 7, 7, 6, 5,
            2, 1, 0, 0, 3, 2,
            5, 6, 0, 0, 1, 5,
            7, 3, 0, 0, 6, 7,
            4, 5, 2, 2, 5, 1
        };

        //std::array<std::string, 6> skyboxCubeImageLocations = { "front.bmp", "back.bmp", "top.bmp", "bottom.bmp", "right.bmp" , "left.bmp" };
        //std::array<std::string, 6> skyboxCubeImageLocations = { "right.png", "left.png", "top.png", "bottom.png", "front.png", "back.png" };

        Image cubeImage;
        std::vector<Texture *> skyboxTextures;

        bool createSkybox();

        bool createDescriptorPool();
        bool createDescriptors();
    public:
        SkyboxPipeline(const std::string name, Renderer * renderer);
        SkyboxPipeline & operator=(SkyboxPipeline) = delete;
        SkyboxPipeline(const SkyboxPipeline&) = delete;
        SkyboxPipeline(SkyboxPipeline &&) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();

        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update();

        ~SkyboxPipeline();
};



#endif


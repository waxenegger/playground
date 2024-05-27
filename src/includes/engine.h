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
        void updateUniformBuffers(int index);

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
        DeviceMemoryUsage getDeviceMemory() const;
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
        PipelineFactory(Renderer* renderer) : renderer(renderer) {}

        template<typename T>
        Pipeline* create(const std::string& name, const PipelineConfig& pipelineConfig)
        {

            if (pipelineConfig.getType() == GenericGraphics) {
                logError("Please instatiate a concrete type instead of a generic!");
                return nullptr;
            }

            std::unique_ptr<Pipeline> pipe = std::make_unique<T>(name, this->renderer);

            if (!pipe->initPipeline(pipelineConfig)) {
                logError("Failed to init Pipeline: " + name);
                return nullptr;
            }

            return pipe.release();
        };
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

        template<typename T>
        bool addPipeline(const std::string name, const PipelineConfig & config, const int index = -1)
        {
            if (this->renderer == nullptr) {
                logError("Engine requires a renderer instance!");
                return false;
            }


            Pipeline * p = this->pipelineFactory->create<T>(name, config);
            if (p == nullptr) {
                logError("Failed to create Pipeline " + name);
                return false;
            }

            std::unique_ptr<Pipeline> sp(p);

            return this->renderer->addPipeline(sp, index);
        };

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
        bool usesDeviceLocalVertexBuffer = false;
        Buffer indexBuffer;
        bool usesDeviceLocalIndexBuffer = false;
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
        std::vector<StaticColorVerticesRenderable *> objectsToBeRendered;
        StaticObjectsColorVertexPipelineConfig config;

    protected:
        bool createBuffers(const ColorVertexPipelineConfig & conf);
        bool createDescriptorPool();
        bool createDescriptors();
        bool addObjectsToBeRendererCommon(const std::vector<ColorVertex> & additionalVertices, const std::vector<uint32_t> & additionalIndices);

    public:
        StaticObjectsColorVertexPipeline(const std::string name, Renderer * renderer);
        StaticObjectsColorVertexPipeline & operator=(StaticObjectsColorVertexPipeline) = delete;
        StaticObjectsColorVertexPipeline(const StaticObjectsColorVertexPipeline&) = delete;
        StaticObjectsColorVertexPipeline(StaticObjectsColorVertexPipeline &&) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();

        bool addObjectsToBeRenderer(const std::vector<StaticColorVerticesRenderable *> & objectsToBeRendered);
        void clearObjectsToBeRenderer();

        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update();

        ~StaticObjectsColorVertexPipeline();
};

class DynamicObjectsColorVertexPipeline : public StaticObjectsColorVertexPipeline {
    private:
        std::vector<DynamicColorVerticesRenderable *> objectsToBeRendered;
        DynamicObjectsColorVertexPipelineConfig config;

    public:
        DynamicObjectsColorVertexPipeline(const std::string name, Renderer * renderer);
        DynamicObjectsColorVertexPipeline & operator=(DynamicObjectsColorVertexPipeline) = delete;
        DynamicObjectsColorVertexPipeline(const DynamicObjectsColorVertexPipeline&) = delete;
        DynamicObjectsColorVertexPipeline(DynamicObjectsColorVertexPipeline &&) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();

        bool addObjectsToBeRenderer(const std::vector<DynamicColorVerticesRenderable *> & additionalObjectsToBeRendered);
        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update();
};

class StaticObjectsColorMeshPipeline : public GraphicsPipeline {
    private:
        std::vector<StaticColorMeshRenderable *> objectsToBeRendered;
        StaticObjectsColorMeshPipelineConfig config;

    protected:
        bool createBuffers(const ColorMeshPipelineConfig & conf);
        bool createDescriptorPool();
        bool createDescriptors();
        bool addObjectsToBeRendererCommon(const std::vector< Vertex >& additionalVertices, const std::vector< uint32_t >& additionalIndices);

    public:
        StaticObjectsColorMeshPipeline(const std::string name, Renderer * renderer);
        StaticObjectsColorMeshPipeline & operator=(StaticObjectsColorMeshPipeline) = delete;
        StaticObjectsColorMeshPipeline(const StaticObjectsColorMeshPipeline&) = delete;
        StaticObjectsColorMeshPipeline(StaticObjectsColorMeshPipeline &&) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();

        bool addObjectsToBeRenderer(const std::vector<StaticColorMeshRenderable *> & objectsToBeRendered);
        void clearObjectsToBeRenderer();

        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update();

        ~StaticObjectsColorMeshPipeline();
};

class DynamicObjectsColorMeshPipeline : public StaticObjectsColorMeshPipeline {
    private:
        std::vector<DynamicColorMeshRenderable *> objectsToBeRendered;
        DynamicObjectsColorMeshPipelineConfig config;

    public:
        DynamicObjectsColorMeshPipeline(const std::string name, Renderer * renderer);
        DynamicObjectsColorMeshPipeline & operator=(DynamicObjectsColorMeshPipeline) = delete;
        DynamicObjectsColorMeshPipeline(const DynamicObjectsColorMeshPipeline&) = delete;
        DynamicObjectsColorMeshPipeline(DynamicObjectsColorMeshPipeline &&) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();

        bool addObjectsToBeRenderer(const std::vector<DynamicColorMeshRenderable *> & additionalObjectsToBeRendered);
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


#ifndef SRC_INCLUDES_RENDERER_INCL_H_
#define SRC_INCLUDES_RENDERER_INCL_H_

#include "graphics.h"

class Renderer final {
    private:
        const VkDeviceSize INDIRECT_DRAW_BUFFER_SIZE_DEFAULT = 50 * MEGA_BYTE;
        const int INDIRECT_DRAW_DEFAULT_NUMBER_OF_BUFFERS = 8;
        const GraphicsContext * graphicsContext = nullptr;
        const VkPhysicalDevice physicalDevice = nullptr;
        VkDevice logicalDevice = nullptr;

        std::map<std::string, uint64_t> deviceProperties;
        bool memoryBudgetExtensionSupported = false;
        bool descriptorIndexingSupported = false;

        CommandPool graphicsCommandPool;
        CommandPool computeCommandPool;

        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkCommandBuffer> computeBuffers;

        uint32_t imageCount = 0;

        int graphicsQueueIndex = -1;
        VkQueue graphicsQueue = nullptr;
        VkQueue altGraphicsQueue = nullptr;
        int computeQueueIndex = -1;
        VkQueue computeQueue = nullptr;

        VkClearColorValue clearValue = BLACK;

        VkPhysicalDeviceMemoryProperties memoryProperties;

        std::vector<Buffer> uniformBuffer;
        std::vector<Buffer> uniformBufferCompute;

        int usedIndirectBufferCount = 0;
        std::vector<Buffer> indirectDrawBuffer;
        VkDeviceSize indirectDrawBufferSize = INDIRECT_DRAW_BUFFER_SIZE_DEFAULT;
        std::vector<bool> usesDeviceIndirectDrawBuffer;
        std::vector<Buffer> indirectDrawCountBuffer;
        std::vector<uint32_t> maxIndirectDrawCount;

        std::vector<Pipeline *> pipelines;

        std::vector<float> deltaTimes;
        float lastDeltaTime = DELTA_TIME_60FPS;
        uint64_t accumulatedDeltaTime = 0;

        std::chrono::high_resolution_clock::time_point lastFrameRateUpdate;
        uint16_t frameRate = FRAME_RATE_60;
        size_t currentFrame = 0;

        bool paused = true;
        bool requiresRenderUpdate = false;
        bool requiresSwapChainRecreate = false;
        bool uploadTexturesToGPU = true;

        bool showWireFrame = false;
        bool minimized = false;
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
        std::vector<VkSemaphore> computeFinishedSemaphores;
        std::vector<VkFence> computeFences;

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
        void computeFrame();

        bool createUniformBuffers();
        void updateUniformBuffers(int index);

        void destroySyncObjects();
        void destroySwapChainObjects(const bool destroyPipelines = true);
        void destroyRendererObjects();

         void setPhysicalDeviceProperties();
    public:
        static glm::vec4 SUN_COLOR_AND_GLOSS;
        static glm::vec4 SUN_LOCATION_STRENGTH;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer &) = delete;
        Renderer(Renderer &&) = delete;
        Renderer & operator=(Renderer) = delete;
        Renderer(const GraphicsContext * graphicsContext, const VkPhysicalDevice & physicalDevice, const int & graphicsQueueIndex, const int & computeQueueIndex);

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

        bool initRenderer();
        bool createRenderer(const bool recreatePipelines = true);
        bool recreatePipelines();

        void forceRenderUpdate(const bool requiresSwapChainRecreate = false);
        void resetRenderUpdate();
        void forceNewTexturesUpload();
        bool doesShowWireFrame() const;
        void setShowWireFrame(bool showWireFrame);
        bool isMinimized() const;

        VkRenderPass getRenderPass() const;
        VkExtent2D getSwapChainExtent() const;

        const CommandPool & getGraphicsCommandPool() const;

        VkQueue getGraphicsQueue() const;
        VkQueue getAltGraphicsQueue() const;
        uint32_t getGraphicsQueueIndex() const;
        VkQueue getComputeQueue() const;
        uint32_t getComputeQueueIndex() const;

        void setClearValue(const VkClearColorValue & clearColorValue);

        Buffer & getIndirectDrawBuffer(const int & index);
        Buffer & getIndirectDrawCountBuffer(const int & index);
        bool createIndirectDrawBuffers();
        void setMaxIndirectCallCount(uint32_t maxIndirectDrawCount, const int & index);
        uint32_t getMaxIndirectCallCount(const int & index);
        int getNextIndirectBufferIndex();

        const VkPhysicalDeviceMemoryProperties & getMemoryProperties() const;
        uint64_t getPhysicalDeviceProperty(const std::string prop) const;
        DeviceMemoryUsage getDeviceMemory() const;
        void trackDeviceLocalMemory(const VkDeviceSize & delta,  const bool & isFree = false);

        Pipeline * getPipeline(const std::string name);

        const Buffer & getUniformBuffer(int index) const;
        const Buffer & getUniformComputeBuffer(int index) const;

        std::vector<MemoryUsage> getMemoryUsage() const;

        void setIndirectDrawBufferSize(const VkDeviceSize & size);

        const GraphicsContext * getGraphicsContext() const;

        void render();

        void waitForQueuesToBeIdle();
        void pause();
        bool isPaused();
        void resume();

        bool isMaximized();
        bool isFullScreen();

        ~Renderer();
};

#endif



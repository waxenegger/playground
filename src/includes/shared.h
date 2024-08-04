#ifndef SRC_INCLUDES_SHARED_INCL_H_
#define SRC_INCLUDES_SHARED_INCL_H_

#include "common.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <SDL_image.h>

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <cmath>
#include <array>

#include <fstream>
#include <random>
#include <any>
#include <variant>
#include <optional>
#include <mutex>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

void signalHandler(int signal);
int start(int argc, char* argv []);

const std::string APP_NAME = "Playground";

constexpr uint32_t VULKAN_VERSION = VK_MAKE_VERSION(1,2,0);
static constexpr VkClearColorValue BLACK = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
static constexpr VkClearColorValue WHITE = {{ 1.0f, 1.0f, 1.0f, 1.0f }};

static const uint32_t MIN_WINDOW_WIDTH = 640;
static const uint32_t MIN_WINDOW_HEIGHT = 480;

static const uint64_t KILO_BYTE = std::pow(2,10);
static const uint64_t MEGA_BYTE = std::pow(2,20);
static const uint64_t GIGA_BYTE = std::pow(2,30);

const VkSurfaceFormatKHR SWAP_CHAIN_IMAGE_FORMAT = {
    #ifndef __ANDROID__
        VK_FORMAT_B8G8R8A8_SRGB,
    #endif
    #ifdef __ANDROID__
        VK_FORMAT_R8G8B8A8_SRGB,
    #endif
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
};

const std::string ALLOCATION_LIMIT = "maxMemoryAllocationCount";
const std::string UNIFORM_BUFFER_LIMIT = "maxUniformBufferRange";
const std::string STORAGE_BUFFER_LIMIT = "maxStorageBufferRange";
const std::string DEVICE_MEMORY_LIMIT = "maxGpuMemory";
const std::string PUSH_CONSTANTS_LIMIT = "maxPushConstantsSize";
const std::string COMPUTE_SHARED_MEMORY_LIMIT = "maxComputeSharedMemorySize";
const std::string DEVICE_MEMORY_INDEX = "gpuMemoryIndex";
const std::string DEVICE_MEMORY_USAGE_MANUALLY_TRACKED = "deviceMemoryManuallyTracked";

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

static constexpr bool USE_GPU_CULLING = true;
static constexpr uint64_t FRAME_RECORDING_INTERVAL = 20;
static constexpr uint32_t FRAME_RECORDING_MAX_FRAMES = 150;

static constexpr uint32_t MAX_NUMBER_OF_TEXTURES = 5000;
static constexpr uint32_t MAX_JOINTS = 250;
static constexpr uint32_t DEFAULT_BUFFERING = 3;
static constexpr uint32_t MIPMAP_LEVELS = 8;

static constexpr int FRAME_RATE_60 = 60;
static constexpr double DELTA_TIME_60FPS = 1000.0f / FRAME_RATE_60;

const float CAMERA_MOVE_INCREMENT = 0.2f;
const float CAMERA_ROTATION_PER_DELTA = glm::radians(45.0f);

struct MemoryUsage {
    std::string name;
    VkDeviceSize vertexBufferUsed = 0;
    VkDeviceSize vertexBufferTotal = 0;
    bool vertexBufferUsesDeviceLocal = false;
    VkDeviceSize indexBufferUsed = 0;
    VkDeviceSize indexBufferTotal = 0;
    bool indexBufferUsesDeviceLocal = false;
    VkDeviceSize instanceDataBufferUsed = 0;
    VkDeviceSize instanceDataBufferTotal = 0;
    VkDeviceSize meshDataBufferUsed = 0;
    VkDeviceSize meshDataBufferTotal = 0;
    VkDeviceSize computeBufferUsed = 0;
    VkDeviceSize computeBufferTotal = 0;
    bool computeBufferUsesDeviceLocal = false;
    VkDeviceSize indirectBufferTotal = 0;
    bool indirectBufferUsesDeviceLocal = false;
};

struct DeviceMemoryUsage {
    VkDeviceSize total;
    VkDeviceSize used;
    VkDeviceSize available;
};

struct GraphicsUniforms {
    glm::mat4 viewProjMatrix;
    glm::vec4 camera;
    glm::vec4 globalLightColorAndGlossiness = glm::vec4(1.0f, 1.0f, 1.0f, 10.0f);
    glm::vec4 globalLightLocationAndStrength = glm::vec4(0.0f, 1000000.0f,1000000.0f, 1.0f);
};

struct CullUniforms {
    std::array<glm::vec4, 6> frustumPlanes;
};

class DescriptorPool final {
    private:
        std::vector<VkDescriptorPoolSize> resources;
        VkDescriptorPool pool = nullptr;
        bool initialized = false;

    public:
        DescriptorPool();
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool &) = delete;
        DescriptorPool(DescriptorPool &&) = delete;
        DescriptorPool & operator=(DescriptorPool) = delete;

        bool isInitialized() const;
        void destroy(const VkDevice & logicalDevice);
        void addResource(const VkDescriptorType type, const uint32_t count);
        void createPool(const VkDevice & logicalDevice, const uint32_t maxSets);
        void resetPool(const VkDevice & logicalDevice);
        const VkDescriptorPool & getPool() const;
        uint32_t getNumberOfResources() const;
};

class Descriptors final {
    private:
        VkDescriptorSetLayout descriptorSetLayout = nullptr;
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        std::vector<VkDescriptorSet> descriptorSets;
        bool initialized = false;

    public:
        Descriptors();
        Descriptors(const Descriptors&) = delete;
        Descriptors& operator=(const Descriptors &) = delete;
        Descriptors(Descriptors &&) = delete;
        Descriptors & operator=(Descriptors) = delete;

        bool isInitialized() const;
        void destroy(const VkDevice & logicalDevice);
        void addBindings(const VkDescriptorType type, const VkShaderStageFlags shaderStageFlags, const uint32_t count);
        void create(const VkDevice & logicalDevice, const VkDescriptorPool & descriptorPool, const uint32_t maxSets);

        const VkDescriptorSetLayout & getDescriptorSetLayout() const;
        const std::vector<VkDescriptorSet> & getDescriptorSets() const;

        void updateWriteDescriptorWithBufferInfo(const VkDevice & logicalDevice, const uint32_t bindingIndex, const uint32_t setIndex, const VkDescriptorBufferInfo & bufferInfo);
        void updateWriteDescriptorWithImageInfo(const VkDevice & logicalDevice, const uint32_t bindingIndex, const uint32_t setIndex, const std::vector<VkDescriptorImageInfo> & descriptorImageInfos);
};

struct ImageConfig {
    VkFormat format = VK_FORMAT_D32_SFLOAT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    int32_t width;
    uint32_t height;
    bool isDepthImage = true;
    VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    uint16_t arrayLayers = 1;
    uint32_t mipLevels = 1;
};

class Image final {
    private:
        VkImage image = nullptr;
        VkDeviceMemory imageMemory = nullptr;
        VkImageView imageView = nullptr;
        VkSampler imageSampler = nullptr;
        bool initialized = false;

        bool getMemoryTypeIndex(const VkPhysicalDevice & physicalDevice, VkMemoryRequirements & memoryRequirements, VkMemoryPropertyFlags properties, uint32_t & memoryTypeIndex);
    public:
        Image();
        Image(const Image&) = delete;
        Image& operator=(const Image &) = delete;
        Image(Image &&) = delete;
        Image & operator=(Image) = delete;

        bool isInitialized() const;
        void destroy(const VkDevice & logicalDevice, const bool isSwapChainImage = false);
        void createImage(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const ImageConfig & config);
        void createFromSwapchainImages(const VkDevice & logicalDevice, const VkImage & image);
        const VkImage & getImage() const;
        const VkImageView & getImageView() const;
        void transitionImageLayout(const VkCommandBuffer& commandBuffer, const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint16_t layerCount = 1, const uint32_t mipLevels = 1) const;
        void copyBufferToImage(const VkCommandBuffer& commandBuffer, const VkBuffer& buffer, const uint32_t width, const uint32_t height, const uint16_t layerCount = 1, const VkImageLayout imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        void generateMipMaps(const VkCommandBuffer& commandBuffer, const int32_t width, const int32_t height, const uint32_t levels) const;
        const VkDescriptorImageInfo getDescriptorInfo() const;
        const VkSampler & getSampler() const;
};

class CommandPool final {
    private:
        VkCommandPool pool = nullptr;
        bool initialized = false;

        VkCommandBuffer beginCommandBuffer(const VkDevice & logicalDevice, const bool primary = true) const;

    public:
        CommandPool();
        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool &) = delete;
        CommandPool(CommandPool &&) = delete;
        CommandPool & operator=(CommandPool) = delete;

        bool isInitialized() const;

        void destroy(const VkDevice & logicalDevice);
        void reset(const VkDevice& logicalDevice);
        void create(const VkDevice & logicalDevice, const int queueIndex = 0);

        VkCommandBuffer beginPrimaryCommandBuffer(const VkDevice & logicalDevice) const;
        VkCommandBuffer beginSecondaryCommandBuffer(const VkDevice & logicalDevice, const VkRenderPass & renderPass) const;

        void endCommandBuffer(const VkCommandBuffer & commandBuffer) const;
        void freeCommandBuffer(const VkDevice & logicalDevice, const VkCommandBuffer & commandBuffer) const;
        void resetCommandBuffer(const VkCommandBuffer & commandBuffer) const;
        void submitCommandBuffer(const VkDevice & logicalDevice, const VkQueue & queue, const VkCommandBuffer & commandBuffer) const;
};

class Buffer final {
    private:
        VkBuffer buffer = nullptr;
        VkDeviceMemory bufferMemory = nullptr;
        void * bufferData = nullptr;
        VkDeviceSize bufferSize = 0;
        VkDeviceSize bufferContentSize = 0;
        bool initialized = false;

        bool getMemoryTypeIndex(const VkPhysicalDevice & physicalDevice, VkMemoryRequirements & memoryRequirements, VkMemoryPropertyFlags properties, uint32_t & memoryTypeIndex);
    public:
        Buffer();
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer &) = delete;
        Buffer(Buffer &&) = delete;
        Buffer & operator=(Buffer) = delete;

        bool isInitialized() const;
        void destroy(const VkDevice & logicalDevice);
        VkResult createBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkBufferUsageFlags usageFlags, const VkDeviceSize & size, const bool isDeviceLocal = false);
        VkResult createIndirectDrawBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size, const bool isDeviceLocal = false);
        VkResult createSharedStorageBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size);
        VkResult createSharedIndexBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size);
        VkResult createSharedUniformBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size);
        VkResult createDeviceLocalBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size,
                                     const VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        VkResult createDeviceLocalBufferFromStagingBuffer(Buffer & stagingBuffer, const VkDeviceSize offset, const VkDeviceSize size,
                                     const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice,
                                     const CommandPool & commandPool, const VkQueue & graphicsQueue,
                                     const VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        VkResult createStagingBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size);
        VkDeviceSize getSize() const;
        VkDeviceSize getContentSize() const;
        void copyBuffer(const CommandPool & commandPool, const VkBuffer & source, const VkDeviceSize size) const;
        void updateContentSize(const VkDeviceSize & contentSize);
        void * getBufferData();
        const VkBuffer& getBuffer() const;
        const VkDeviceMemory & getBufferMemory() const;
        const VkDescriptorBufferInfo getDescriptorInfo() const;
};

class Renderable;
class Camera final
{
    public:
        enum CameraMode { lookat, firstperson };
        enum KeyPress { LEFT = 0, RIGHT = 1, UP = 2, DOWN = 3, NONE = 4 };

        const float DefaultThirdPersonCameraDistance = 25.0f;

        glm::vec3 getPosition();

        void setAspectRatio(float aspect);

        void setFovY(float degrees);
        float getFovY();

        void setPerspective();

        void setPosition(glm::vec3 position);
        void setRotation(glm::vec3 rotation);
        glm::vec3 & getRotation();

        void update(const float deltaTime = DELTA_TIME_60FPS);

        glm::mat4 getModelMatrix();
        glm::mat4 getViewMatrix();
        glm::mat4 getProjectionMatrix();

        static Camera * INSTANCE(glm::vec3 pos);
        static Camera * INSTANCE();

        void move(KeyPress key, bool isPressed = false);

        void rotate(const float deltaX, const float  deltaY);
        void accumulateRotationDeltas(const float deltaX, const float  deltaY);

        const std::array<glm::vec4, 6> & getFrustumPlanes();
        const std::array<glm::vec4, 6> calculateFrustum(const glm::mat4 & matrix);

        glm::vec3 getCameraFront();
        void updateFrustum();
        void destroy();

        void linkToRenderable(Renderable * renderable);
        bool isInThirdPersonMode();

    private:
        Renderable * linkedRenderable = nullptr;

        Camera(glm::vec3 position);

        static Camera * instance;
        CameraMode mode = CameraMode::firstperson;

        std::array<glm::vec4, 6> frustumPlanes;
        glm::vec3 position = glm::vec3();
        glm::vec3 rotation = glm::vec3();

        float deltaX = 0;
        float deltaY = 0;

        float aspect = 1.0f;
        float fovy = 45.0f;

        Direction keys;

        glm::mat4 perspective = glm::mat4();
        glm::mat4 view = glm::mat4();

        void updateViewMatrix();
        bool moving();
};

class KeyValueStore final {
    private:
        KeyValueStore();
        static std::unordered_map<std::string, std::any> map;
    public:
        KeyValueStore& operator=(const KeyValueStore &) = delete;
        KeyValueStore(KeyValueStore &&) = delete;
        KeyValueStore & operator=(KeyValueStore) = delete;

        template <typename T>
        static T getValue(const std::string &key, T defaultValue)
        {
            auto it = map.find(key);
            if (it == map.end() || !it->second.has_value()) return defaultValue;

            T ret;
            try {
                ret = std::any_cast<T>(it->second);
            } catch(std::bad_any_cast ex) {
                logError("Failed to cast map value to given type!");
                return defaultValue;
            }

            return ret;
        };

        template <typename T>
        static void setValue(const std::string key, T value)
        {
            map[key] = value;
        };
};

#endif

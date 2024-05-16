#ifndef SRC_INCLUDES_SHARED_INCL_H_
#define SRC_INCLUDES_SHARED_INCL_H_

#include "logging.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <SDL_image.h>

#include <vulkan/vulkan.h>

#include <zmq.h>

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <filesystem>
#include <map>
#include <array>
#include <fstream>
#include <random>
#include <any>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/string_cast.hpp>
//#include <gtc/quaternion.hpp>
//#include <gtx/quaternion.hpp>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

int start(int argc, char* argv []);

const std::string APP_NAME = "Playground";

constexpr uint32_t VULKAN_VERSION = VK_MAKE_VERSION(1,2,0);
static constexpr VkClearColorValue BLACK = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
static constexpr VkClearColorValue WHITE = {{ 1.0f, 1.0f, 1.0f, 1.0f }};

const VkSurfaceFormatKHR SWAP_CHAIN_IMAGE_FORMAT = {
    #ifndef __ANDROID__
        VK_FORMAT_B8G8R8A8_SRGB,
    #endif
    #ifdef __ANDROID__
        VK_FORMAT_R8G8B8A8_SRGB,
    #endif
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
};

struct Direction final {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
};

struct GraphicsUniforms {
    glm::mat4 viewProjMatrix;
    glm::vec4 camera;
};

enum APP_PATHS {
    ROOT, TEMP, SHADERS, MODELS, SKYBOX, FONTS, MAPS
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

static constexpr float PI_HALF = glm::pi<float>() / 2;
static constexpr float INF = std::numeric_limits<float>::infinity();
static constexpr float NEG_INF = - std::numeric_limits<float>::infinity();

static constexpr uint32_t DEFAULT_BUFFERING = 3;
static constexpr uint32_t MIPMAP_LEVELS = 8;

static constexpr int FRAME_RATE_60 = 60;
static constexpr double DELTA_TIME_60FPS = 1000.0f / FRAME_RATE_60;

const float CAMERA_MOVE_INCREMENT = 0.2f;
const float CAMERA_ROTATION_PER_DELTA = glm::radians(45.0f);

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
        void createImage(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkFormat format, const VkImageUsageFlags usage, const int32_t width, const uint32_t height, bool isDepthImage = false, const VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, const uint16_t arrayLayers = 1, const uint32_t mipLevels = 1);
        void createFromSwapchainImages(const VkDevice & logicalDevice, const VkImage & image);
        const VkImage & getImage() const;
        const VkImageView & getImageView() const;
        void transitionImageLayout(const VkCommandBuffer& commandBuffer, const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint16_t layerCount = 1, const uint32_t mipLevels = 1) const;
        void copyBufferToImage(const VkCommandBuffer& commandBuffer, const VkBuffer & buffer, const uint32_t width, const uint32_t height, const uint16_t layerCount = 1) const;
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
        VkResult createDeviceLocalBuffer(Buffer & stagingBuffer, const VkDeviceSize offset, const VkDeviceSize size,
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

class Camera final
{
    public:
        enum CameraType { lookat, firstperson };
        enum KeyPress { LEFT = 0, RIGHT = 1, UP = 2, DOWN = 3, NONE = 4 };
        glm::vec3 getPosition();
        void setAspectRatio(float aspect);
        void setFovY(float degrees);
        float getFovY();
        void setPerspective();
        void setPosition(glm::vec3 position);
        void setRotation(glm::vec3 rotation);
        void placeCamera(float x, float y, float z);
        glm::vec3 & getRotation();
        void update(const float deltaTime = DELTA_TIME_60FPS);
        glm::mat4 getModelMatrix();
        glm::mat4 getViewMatrix();
        glm::mat4 getProjectionMatrix();
        static Camera * INSTANCE(glm::vec3 pos);
        static Camera * INSTANCE();
        void setType(CameraType type);
        void move(KeyPress key, bool isPressed = false);
        void rotate(const float deltaX, const float  deltaY);
        void accumulateRotationDeltas(const float deltaX, const float  deltaY);
        const std::array<glm::vec4, 6> & getFrustumPlanes();
        const std::array<glm::vec4, 6> calculateFrustum(const glm::mat4 & matrix);
        glm::vec3 getCameraFront();
        void updateFrustum();
        void destroy();
    private:
        Camera(glm::vec3 position);

        static Camera * instance;
        CameraType type = CameraType::firstperson;

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

class Helper final {
    private:
        static std::default_random_engine default_random_engine;
        static std::uniform_real_distribution<float> distribution;

    public:
        Helper(const Helper&) = delete;
        Helper& operator=(const Helper &) = delete;
        Helper(Helper &&) = delete;
        Helper & operator=(Helper) = delete;

        static bool findMemoryType(const VkPhysicalDeviceMemoryProperties & memProperties, uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t & memoryType);
        static std::string formatMemoryUsage(const VkDeviceSize size);

        static float getRandomFloatBetween0and1();
        static uint64_t getTimeInMillis();

        static std::vector<std::tuple<std::string, float>> getCameraCrossHairIntersection();
};

class KeyValueStore final {
    private:
        KeyValueStore();
        static std::map<std::string, std::any> map;
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

#ifndef SRC_INCLUDES_SHARED_INCL_H_
#define SRC_INCLUDES_SHARED_INCL_H_

#include "logging.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <zmq.h>

#include <vector>
#include <string>
#include <memory>
#include <thread>

int start(int argc, char* argv []);

constexpr uint32_t VULKAN_VERSION = VK_MAKE_VERSION(1,2,0);
const std::string APP_NAME = "Playground";

const VkSurfaceFormatKHR SWAP_CHAIN_IMAGE_FORMAT = {
    #ifndef __ANDROID__
        VK_FORMAT_B8G8R8A8_SRGB,
    #endif
    #ifdef __ANDROID__
        VK_FORMAT_R8G8B8A8_SRGB,
    #endif
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
};

/*
enum APP_PATHS {
    ROOT, TEMP, SHADERS, MODELS, SKYBOX, FONTS, MAPS
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

class Image final {
    private:
        VkImage image = nullptr;
        VkDeviceMemory imageMemory = nullptr;
        VkImageView imageView = nullptr;
        VkSampler imageSampler = nullptr;
        bool initialized = false;

    public:
        Image();
        Image(const Image&) = delete;
        Image& operator=(const Image &) = delete;
        Image(Image &&) = delete;
        Image & operator=(Image) = delete;

        bool isInitialized() const;
        void destroy(const VkDevice & logicalDevice, const bool isSwapChainImage = false);
        void createImage(const Renderer * renderer, const VkFormat format, const VkImageUsageFlags usage, const int32_t width, const uint32_t height, bool isDepthImage = false, const VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, const uint16_t arrayLayers = 1, const uint32_t mipLevels = 1);
        void createFromSwapchainImages(const Renderer * renderer, const VkImage & image);
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
        void create(const Renderer * renderer, const bool forComputeQueue = false);

        VkCommandBuffer beginPrimaryCommandBuffer(const VkDevice & logicalDevice) const;
        VkCommandBuffer beginSecondaryCommandBuffer(const Renderer * renderer) const;

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

    public:
        Buffer();
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer &) = delete;
        Buffer(Buffer &&) = delete;
        Buffer & operator=(Buffer) = delete;

        bool isInitialized() const;
        void destroy(const VkDevice & logicalDevice);
        void createBuffer(const Renderer * renderer, const VkBufferUsageFlags usageFlags, const VkDeviceSize & size, const bool isDeviceLocal = false);
        void createIndirectDrawBuffer(const Renderer * renderer, const VkDeviceSize size, const bool isDeviceLocal = false);
        void createSharedStorageBuffer(const Renderer * renderer, const VkDeviceSize size);
        void createSharedIndexBuffer(const Renderer * renderer, const VkDeviceSize size);
        void createSharedUniformBuffer(const Renderer * renderer, const VkDeviceSize size);
        void createDeviceLocalBuffer(const Renderer * renderer, Buffer & stagingBuffer, const VkDeviceSize offset, const VkDeviceSize size, const VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, const bool forCompute = false);
        void createStagingBuffer(const Renderer * renderer, const VkDeviceSize size);
        VkDeviceSize getSize() const;
        VkDeviceSize getContentSize() const;
        void copyBuffer(const CommandPool & commandPool, const VkBuffer & source, const VkDeviceSize size) const;
        void updateContentSize(const VkDeviceSize & contentSize);
        void * getBufferData();
        const VkBuffer& getBuffer() const;
        const VkDeviceMemory & getBufferMemory() const;
        const VkDescriptorBufferInfo getDescriptorInfo() const;
};
*/

#endif

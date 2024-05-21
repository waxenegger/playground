#include "includes/shared.h"

DescriptorPool::DescriptorPool() {}

bool DescriptorPool::isInitialized() const
{
    return this->initialized;
}

void DescriptorPool::addResource(const VkDescriptorType type, const uint32_t count)
{
    const VkDescriptorPoolSize & resource = { type, count };
    this->resources.push_back(resource);
}

void DescriptorPool::createPool(const VkDevice & logicalDevice, const uint32_t maxSets)
{
    if (this->resources.empty()) return;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(this->resources.size());
    poolInfo.pPoolSizes = this->resources.data();
    poolInfo.maxSets = maxSets;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkResult ret = vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &this->pool);
    if (ret != VK_SUCCESS) {
       logError("Failed to Create Descriptor Pool!");
       return;
    }

    this->initialized = true;
}

void DescriptorPool::destroy(const VkDevice & logicalDevice)
{
    this->initialized = false;
    this->resources.clear();

    if (this->pool != nullptr) {
        vkDestroyDescriptorPool(logicalDevice, this->pool, nullptr);
        this->pool = nullptr;
    }
}

void DescriptorPool::resetPool(const VkDevice & logicalDevice)
{
    if (this->initialized) {
        vkResetDescriptorPool(logicalDevice, this->pool, VK_NULL_HANDLE);
    }
}

const VkDescriptorPool & DescriptorPool::getPool() const
{
    return this->pool;
}

uint32_t DescriptorPool::getNumberOfResources() const
{
    return this->resources.size();
}

Descriptors::Descriptors() {}

bool Descriptors::isInitialized() const
{
    return this->initialized;
}

void Descriptors::addBindings(const VkDescriptorType type, const VkShaderStageFlags shaderStageFlags, const uint32_t count)
{
    VkDescriptorSetLayoutBinding layoutBinding {};
    layoutBinding.binding = static_cast<uint32_t>(this->layoutBindings.size());
    layoutBinding.descriptorCount = count;
    layoutBinding.descriptorType = type;
    layoutBinding.pImmutableSamplers = nullptr;
    layoutBinding.stageFlags = shaderStageFlags;
    this->layoutBindings.push_back(layoutBinding);
}

void Descriptors::destroy(const VkDevice & logicalDevice)
{
    this->initialized = false;

    if (this->descriptorSetLayout != nullptr) {
        vkDestroyDescriptorSetLayout(logicalDevice, this->descriptorSetLayout, nullptr);
        this->descriptorSetLayout = nullptr;
    }

    this->layoutBindings.clear();
    this->descriptorSets.clear();
}

const std::vector<VkDescriptorSet> & Descriptors::getDescriptorSets() const
{
    return this->descriptorSets;
}


void Descriptors::create(const VkDevice & logicalDevice, const VkDescriptorPool & descriptorPool, const uint32_t maxSets)
{
    if (this->layoutBindings.empty()) return;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = this->layoutBindings.size();
    layoutInfo.pBindings = this->layoutBindings.data();

    VkResult ret = vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, 0, &this->descriptorSetLayout);
    if (ret != VK_SUCCESS) {
        logError("Failed to Create Descriptor Set Layout!");
        return;
    }

    std::vector<VkDescriptorSetLayout> layouts(maxSets, this->descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = maxSets;
    allocInfo.pSetLayouts = layouts.data();

    this->descriptorSets.resize(maxSets);
    ret = vkAllocateDescriptorSets(logicalDevice, &allocInfo, this->descriptorSets.data());
    if (ret != VK_SUCCESS) {
        logError("Failed to Allocate Pipeline Descriptor Sets!");
        this->destroy(logicalDevice);
        return;
    }


   this->initialized = true;
}

const VkDescriptorSetLayout & Descriptors::getDescriptorSetLayout() const
{
    return this->descriptorSetLayout;
}

void Descriptors::updateWriteDescriptorWithBufferInfo(const VkDevice & logicalDevice, const uint32_t bindingIndex, const uint32_t setIndex, const VkDescriptorBufferInfo & bufferInfo)
{
    if (!this->initialized || bindingIndex >= this->layoutBindings.size()) return;

        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = this->descriptorSets[setIndex];
        writeDescriptorSet.dstBinding = bindingIndex;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = this->layoutBindings[bindingIndex].descriptorType;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
}

void Descriptors::updateWriteDescriptorWithImageInfo(const VkDevice & logicalDevice, const uint32_t bindingIndex, const uint32_t setIndex, const std::vector<VkDescriptorImageInfo> & descriptorImageInfos)
{
    if (!this->initialized || bindingIndex >= this->layoutBindings.size()) return;

        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = this->descriptorSets[setIndex];
        writeDescriptorSet.dstBinding = bindingIndex;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = this->layoutBindings[bindingIndex].descriptorType;
        writeDescriptorSet.descriptorCount = descriptorImageInfos.size();
        writeDescriptorSet.pImageInfo = descriptorImageInfos.data();

        vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
}

Buffer::Buffer() {}

const VkDescriptorBufferInfo Buffer::getDescriptorInfo() const
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = this->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = this->bufferSize;

    return bufferInfo;
}

VkResult Buffer::createBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkBufferUsageFlags usageFlags, const VkDeviceSize & size, const bool isDeviceLocal) {
    if (size == 0) return VK_ERROR_UNKNOWN;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult ret = vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &this->buffer);
    if (ret != VK_SUCCESS) {
        logError("Failed to get Create Buffer!");
        return ret;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logicalDevice, this->buffer, &memRequirements);

    VkMemoryPropertyFlags memoryPreference =
        isDeviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT :
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    uint32_t memoryTypeIndex;
    if (!Helper::getMemoryTypeIndex(physicalDevice, memRequirements, memoryPreference, memoryPreference, memoryTypeIndex)) {
            this->destroy(logicalDevice);
            logError("Failed to get Memory Type Requested!");
            return VK_ERROR_UNKNOWN ;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    ret = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &this->bufferMemory);
    if (ret != VK_SUCCESS) {
        this->destroy(logicalDevice);
        logError("Failed to Allocate Memory for Buffer!");
        return ret;
    }
    this->bufferSize = size;

    vkBindBufferMemory(logicalDevice, this->buffer, this->bufferMemory, 0);

    if (!isDeviceLocal) {
        vkMapMemory(logicalDevice, this->bufferMemory, 0, this->bufferSize, 0, &this->bufferData);
    }

    this->initialized = true;

    return VK_SUCCESS;
};

VkResult Buffer::createIndirectDrawBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size, const bool isDeviceLocal) {
    return this->createBuffer(physicalDevice, logicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, size, isDeviceLocal);
};

VkResult Buffer::createSharedStorageBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size) {
    return this->createBuffer(physicalDevice, logicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size);
};

VkResult Buffer::createSharedIndexBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size) {
    return this->createBuffer(physicalDevice, logicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
};

VkResult Buffer::createSharedUniformBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size) {
    return this->createBuffer(physicalDevice, logicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size);
};

VkResult Buffer::createDeviceLocalBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size,const VkBufferUsageFlagBits usage)
{
    return this->createBuffer(physicalDevice, logicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, size, true);
}

VkResult Buffer::createDeviceLocalBufferFromStagingBuffer(Buffer& stagingBuffer, const VkDeviceSize offset, const VkDeviceSize size, const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const CommandPool & commandPool, const VkQueue & graphicsQueue, VkBufferUsageFlagBits usage)
{
    VkResult res = this->createBuffer(physicalDevice, logicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, size, true);
    if (this->initialized) {

        if (!stagingBuffer.isInitialized() || stagingBuffer.getContentSize() == 0 || stagingBuffer.getContentSize() > this->bufferSize) {
            logError("Staging Buffer must be intialized and smaller than device local buffer it is copied into!");
            return res;
        }

        const VkCommandBuffer & commandBuffer = commandPool.beginPrimaryCommandBuffer(logicalDevice);

        VkBufferCopy copyRegion {};
        copyRegion.srcOffset = offset;
        copyRegion.dstOffset = offset;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, stagingBuffer.getBuffer(), this->buffer, 1, &copyRegion);

        commandPool.endCommandBuffer(commandBuffer);
        commandPool.submitCommandBuffer(logicalDevice, graphicsQueue, commandBuffer);
        this->bufferContentSize = this->bufferSize;
    }

    return res;
};

VkResult Buffer::createStagingBuffer(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const VkDeviceSize size) {
    return this->createBuffer(physicalDevice, logicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size);
};

bool Buffer::isInitialized() const
{
    return this->initialized;
}

VkDeviceSize Buffer::getSize() const
{
    return this->bufferSize;
}

void Buffer::updateContentSize(const VkDeviceSize & contentSize)
{
    this->bufferContentSize = contentSize;
}

VkDeviceSize Buffer::getContentSize() const
{
    return this->bufferContentSize;
}

void Buffer::destroy(const VkDevice & logicalDevice) {
    if (logicalDevice == nullptr) return;

    this->initialized = false;
    this->bufferSize = 0;
    this->bufferContentSize = 0;

    if (this->bufferData != nullptr) {
        vkUnmapMemory(logicalDevice, this->bufferMemory);
        this->bufferData = nullptr;
    }

    if (this->buffer != nullptr) {
        vkDestroyBuffer(logicalDevice, this->buffer, nullptr);
        this->buffer = nullptr;
    }

    if (this->bufferMemory != nullptr) {
        vkFreeMemory(logicalDevice, this->bufferMemory, nullptr);
        this->bufferMemory = nullptr;
    }
}

void * Buffer::getBufferData()
{
    if (!this->initialized) return nullptr;

    return this->bufferData;
}

const VkBuffer & Buffer::getBuffer() const
{
    return this->buffer;
}

const VkDeviceMemory & Buffer::getBufferMemory() const
{
    return this->bufferMemory;
}


Image::Image() {}

bool Image::isInitialized() const
{
    return this->initialized;
}

const VkDescriptorImageInfo Image::getDescriptorInfo() const
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = this->imageSampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = this->imageView;

    return imageInfo;
}

void Image::createImage(const VkPhysicalDevice & physicalDevice, const VkDevice & logicalDevice, const ImageConfig & config)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = config.width;
    imageInfo.extent.height = config.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = config.mipLevels;
    imageInfo.arrayLayers = config.arrayLayers;
    imageInfo.format = config.format;
    imageInfo.tiling = config.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = config.usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (config.arrayLayers > 1) {
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VkResult ret = vkCreateImage(logicalDevice, &imageInfo, nullptr, &this->image);
    if (ret != VK_SUCCESS) {
        logError("Failed to Create Image");
        return;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logicalDevice, this->image, &memRequirements);

    VkMemoryPropertyFlags alternativeFlags =
        (config.memoryFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ?
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    uint32_t memoryTypeIndex;
    if (!Helper::getMemoryTypeIndex(physicalDevice, memRequirements, config.memoryFlags, alternativeFlags, memoryTypeIndex)) {
        this->destroy(logicalDevice);
        logError("Failed to get Image Memory Type Requested");
        return;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    ret = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &this->imageMemory);
    if (ret != VK_SUCCESS) {
        this->destroy(logicalDevice);
        logError("Failed to Allocate Image Memory");
        return;
    }

    vkBindImageMemory(logicalDevice, this->image, this->imageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = this->image;
    viewInfo.viewType = config.arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = config.format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = config.isDepthImage ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = config.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = config.arrayLayers;

    ret = vkCreateImageView(logicalDevice, &viewInfo, nullptr, &this->imageView);
    if (ret != VK_SUCCESS) {
        this->destroy(logicalDevice);
        logError("Failed to Create Image View!");
        return;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = config.addressMode;
    samplerInfo.addressModeV = config.addressMode;
    samplerInfo.addressModeW = config.addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = MIPMAP_LEVELS;
    samplerInfo.mipLodBias = 0.0f;

    ret = vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &this->imageSampler);
    if (ret != VK_SUCCESS) {
        this->destroy(logicalDevice);
        logError("Failed to Create Image Sampler!");
        return;
    }

    this->initialized = true;
}

const VkSampler& Image::getSampler() const
{
    return this->imageSampler;
}

void Image::destroy(const VkDevice & logicalDevice, const bool isSwapChainImage) {
    if (logicalDevice == nullptr) return;

    this->initialized = false;

    if (this->imageView != nullptr) {
        vkDestroyImageView(logicalDevice, this->imageView, nullptr);
        this->imageView = nullptr;
    }

    if (!isSwapChainImage && this->image != nullptr) {
        vkDestroyImage(logicalDevice, this->image, nullptr);
        this->image = nullptr;
    }

    if (!isSwapChainImage && this->imageMemory != nullptr) {
        vkFreeMemory(logicalDevice, this->imageMemory, nullptr);
        this->imageMemory = nullptr;
    }

    if (!isSwapChainImage && this->imageSampler != nullptr) {
        vkDestroySampler(logicalDevice, this->imageSampler, nullptr);
    }
}

const VkImage & Image::getImage() const
{
    return this->image;
}

const VkImageView & Image::getImageView() const
{
    return this->imageView;
}

void Image::createFromSwapchainImages(const VkDevice & logicalDevice, const VkImage & image)
{
    this->destroy(logicalDevice);

    this->image = image;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = this->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = SWAP_CHAIN_IMAGE_FORMAT.format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkResult ret = vkCreateImageView(logicalDevice, &viewInfo, nullptr, &this->imageView);
    if (ret != VK_SUCCESS) {
        this->destroy(logicalDevice);
        logError("Failed to Create Swap Chain Image View!");
        return;
    }

    this->initialized = true;
}

void Image::transitionImageLayout(const VkCommandBuffer & commandBuffer, const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint16_t layerCount, const uint32_t mipLevels) const
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = this->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == newLayout) {
        return;
    } else
    {
        logError("Unsupported Layout Transition");
        return;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void Image::copyBufferToImage(const VkCommandBuffer& commandBuffer, const VkBuffer & buffer, const uint32_t width, const uint32_t height, const uint16_t layerCount) const
{
    std::vector<VkBufferImageCopy> regions;
    VkBufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = { width, height, 1};
    regions.push_back(region);

    vkCmdCopyBufferToImage(commandBuffer, buffer, this->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());

}

void Image::generateMipMaps(const VkCommandBuffer& commandBuffer, const int32_t width, const int32_t height, const uint32_t levels) const
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = this->image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = width;
    int32_t mipHeight = height;

    for (uint32_t i = 1; i < levels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}

CommandPool::CommandPool() {}

bool CommandPool::isInitialized() const
{
    return this->initialized;
}

void CommandPool::destroy(const VkDevice& logicalDevice)
{
    if (logicalDevice == nullptr || !this->initialized) return;

    this->initialized = false;

    vkDestroyCommandPool(logicalDevice, this->pool, nullptr);
    this->pool = nullptr;
}

void CommandPool::reset(const VkDevice& logicalDevice) {
    if (logicalDevice == nullptr || this->pool == nullptr) return;

    vkResetCommandPool(logicalDevice, this->pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

void CommandPool::create(const VkDevice & logicalDevice, const int queueIndex)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = queueIndex;

    VkResult ret = vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &this->pool);
    if (ret != VK_SUCCESS) {
        logError("Failed to Create Command Pool!");
        return;
    }

    initialized = true;
}

void CommandPool::freeCommandBuffer(const VkDevice & logicalDevice, const VkCommandBuffer & commandBuffer) const
{
    if (!this->initialized) return;

    vkFreeCommandBuffers(logicalDevice, this->pool, 1, &commandBuffer);
}

void CommandPool::resetCommandBuffer(const VkCommandBuffer& commandBuffer) const
{
    if (!this->initialized) return;

    vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
}

VkCommandBuffer CommandPool::beginPrimaryCommandBuffer(const VkDevice & logicalDevice) const
{
    VkCommandBuffer commandBuffer = this->beginCommandBuffer(logicalDevice);
    if (commandBuffer == nullptr) return nullptr;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = VK_NULL_HANDLE;

    VkResult ret = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (ret != VK_SUCCESS) {
        this->freeCommandBuffer(logicalDevice, commandBuffer);
        logError("Failed to Begin Primary Command Buffer!");
        return nullptr;
    }

    return commandBuffer;
}


VkCommandBuffer CommandPool::beginCommandBuffer(const VkDevice & logicalDevice, const bool primary) const
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandPool = this->pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = nullptr;
    VkResult ret = vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);
    if (ret != VK_SUCCESS) {
        logError("Failed to Allocate Command Buffer!");
        return nullptr;
    }

    return commandBuffer;
}

VkCommandBuffer CommandPool::beginSecondaryCommandBuffer(const VkDevice & logicalDevice, const VkRenderPass & renderPass) const
{
    VkCommandBuffer commandBuffer = this->beginCommandBuffer(logicalDevice, false);
    if (commandBuffer == nullptr) return nullptr;

    VkCommandBufferInheritanceInfo inheritenceInfo {};
    inheritenceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritenceInfo.renderPass = renderPass;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = &inheritenceInfo;

    VkResult ret = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (ret != VK_SUCCESS) {
        this->freeCommandBuffer(logicalDevice, commandBuffer);
        logError("Failed to Begin Secondary Command Buffer!");
        return nullptr;
    }

    return commandBuffer;
}

void CommandPool::endCommandBuffer(const VkCommandBuffer & commandBuffer) const
{
    VkResult ret = vkEndCommandBuffer(commandBuffer);
    if (ret != VK_SUCCESS) {
        logError("Failed to End Command Buffer!");
        return;
    }
}

void CommandPool::submitCommandBuffer(const VkDevice & logicalDevice, const VkQueue & queue, const VkCommandBuffer & commandBuffer) const
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    this->freeCommandBuffer(logicalDevice, commandBuffer);
}

std::map<std::string, std::any> KeyValueStore::map;

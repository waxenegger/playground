#include "includes/engine.h"

StaticObjectsColorVertexPipeline::StaticObjectsColorVertexPipeline(const std::string name, Renderer * renderer) : GraphicsPipeline(name, renderer) { }

bool StaticObjectsColorVertexPipeline::createBuffers()
{
    if (this->config.colorVertices.empty()) return true;

    VkResult result;

    VkDeviceSize contentSize = this->config.colorVertices.size() * sizeof(ColorVertex);
    VkDeviceSize reservedSize =
        (this->config.reservedVertexSpace > 0 && this->config.reservedVertexSpace > contentSize) ?
            this->config.reservedVertexSpace : contentSize;

    this->usesDeviceLocalVertexBuffer = this->renderer->getDeviceMemory().available >= reservedSize;

    uint64_t limit = this->usesDeviceLocalVertexBuffer ?
        this->renderer->getPhysicalDeviceProperty(ALLOCATION_LIMIT) :
         this->renderer->getPhysicalDeviceProperty(STORAGE_BUFFER_LIMIT);

    if (reservedSize > limit) {
        logError("You tried to allocate more in one go than the GPU's allocation/storage buffer limit");
        return false;
    }

    if (this->usesDeviceLocalVertexBuffer) this->renderer->trackDeviceLocalMemory(this->vertexBuffer.getSize(), true);
    this->vertexBuffer.destroy(this->renderer->getLogicalDevice());

    if (this->usesDeviceLocalVertexBuffer) {
        result = this->vertexBuffer.createDeviceLocalBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);

        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            this->usesDeviceLocalVertexBuffer = false;
        } else {
            this->renderer->trackDeviceLocalMemory(this->vertexBuffer.getSize());
        }
    }

    if (!this->usesDeviceLocalVertexBuffer) {
        result = this->vertexBuffer.createSharedStorageBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);
    }

    if (!this->vertexBuffer.isInitialized()) {
        logError("Failed to create StaticObjectsColorVertexPipeline Vertex Buffer!");
        return false;
    }

    if (this->config.indices.empty()) return true;

    contentSize = this->config.indices.size() * sizeof(uint32_t);
    reservedSize =
    (this->config.reservedIndexSpace > 0 && this->config.reservedIndexSpace > contentSize) ?
        this->config.reservedIndexSpace : contentSize;

    limit = this->usesDeviceLocalIndexBuffer ?
        this->renderer->getPhysicalDeviceProperty(ALLOCATION_LIMIT) :
         this->renderer->getPhysicalDeviceProperty(STORAGE_BUFFER_LIMIT);

    if (reservedSize > limit) {
        logError("You tried to allocate more in one go than the GPU's allocation/storage buffer limit");
        return false;
    }

    this->usesDeviceLocalIndexBuffer = this->renderer->getDeviceMemory().available >= reservedSize;

    if (this->usesDeviceLocalIndexBuffer) this->renderer->trackDeviceLocalMemory(this->indexBuffer.getSize(), true);
    this->indexBuffer.destroy(this->renderer->getLogicalDevice());

    if (this->usesDeviceLocalIndexBuffer) {
        result = this->indexBuffer.createDeviceLocalBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            this->usesDeviceLocalIndexBuffer = false;
        } else {
            this->renderer->trackDeviceLocalMemory(this->indexBuffer.getSize());
        }
    }

    if (!this->usesDeviceLocalIndexBuffer) {
        result = this->indexBuffer.createSharedIndexBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);
    }

    if (!this->indexBuffer.isInitialized()) {
        logError("Failed to create StaticObjectsColorVertexPipeline Index Buffer!");
        return false;
    }

    return true;
}

bool StaticObjectsColorVertexPipeline::updateBuffers() {

    Buffer stagingBuffer;
    VkDeviceSize contentSize;

    if (this->vertexBuffer.isInitialized() && !this->config.colorVertices.empty()) {
        contentSize = this->config.colorVertices.size() * sizeof(ColorVertex);
        if (contentSize > this->vertexBuffer.getSize()) {
            logError("Can not update buffer since size is too small!");
            return false;
        }

        if (this->usesDeviceLocalVertexBuffer) {
            stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), contentSize);
            if (stagingBuffer.isInitialized()) {
                stagingBuffer.updateContentSize(contentSize);
                memcpy(static_cast<char *>(stagingBuffer.getBufferData()), this->config.colorVertices.data(), stagingBuffer.getContentSize());

                const CommandPool & pool = renderer->getGraphicsCommandPool();
                const VkCommandBuffer & commandBuffer = pool.beginPrimaryCommandBuffer(renderer->getLogicalDevice());

                VkBufferCopy copyRegion {};
                copyRegion.srcOffset = 0;
                copyRegion.dstOffset = 0;
                copyRegion.size = contentSize;
                vkCmdCopyBuffer(commandBuffer, stagingBuffer.getBuffer(), this->vertexBuffer.getBuffer(), 1, &copyRegion);

                pool.endCommandBuffer(commandBuffer);
                pool.submitCommandBuffer(renderer->getLogicalDevice(), renderer->getGraphicsQueue(), commandBuffer);

                stagingBuffer.destroy(this->renderer->getLogicalDevice());

                this->vertexBuffer.updateContentSize(contentSize);
            }
        } else {
            memcpy(static_cast<char *>(this->vertexBuffer.getBufferData()), this->config.colorVertices.data(), contentSize);
            this->vertexBuffer.updateContentSize(contentSize);
        }
    }

    if (this->indexBuffer.isInitialized() && !this->config.indices.empty()) {
        contentSize = this->config.indices.size() * sizeof(uint32_t);

        if (contentSize > this->indexBuffer.getSize()) {
            logError("Can not update buffer since size is too small!");
            return false;
        }

        if (this->usesDeviceLocalIndexBuffer) {
            stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), contentSize);
            if (stagingBuffer.isInitialized()) {
                stagingBuffer.updateContentSize(contentSize);
                memcpy(static_cast<char *>(stagingBuffer.getBufferData()), this->config.indices.data(), stagingBuffer.getContentSize());

                const CommandPool & pool = renderer->getGraphicsCommandPool();
                const VkCommandBuffer & commandBuffer = pool.beginPrimaryCommandBuffer(renderer->getLogicalDevice());

                VkBufferCopy copyRegion {};
                copyRegion.srcOffset = 0;
                copyRegion.dstOffset = 0;
                copyRegion.size = contentSize;
                vkCmdCopyBuffer(commandBuffer, stagingBuffer.getBuffer(), this->indexBuffer.getBuffer(), 1, &copyRegion);

                pool.endCommandBuffer(commandBuffer);
                pool.submitCommandBuffer(renderer->getLogicalDevice(), renderer->getGraphicsQueue(), commandBuffer);

                stagingBuffer.destroy(this->renderer->getLogicalDevice());

                this->indexBuffer.updateContentSize(contentSize);
            }
        } else {
            memcpy(static_cast<char *>(this->indexBuffer.getBufferData()), this->config.indices.data(), contentSize);
            this->indexBuffer.updateContentSize(contentSize);
        }
    }

    return true;
}

bool StaticObjectsColorVertexPipeline::createPipeline()
{
    if (!this->createDescriptors()) {
        logError("Failed to create Models Pipeline Descriptors");
        return false;
    }

    return this->createGraphicsPipelineCommon(true, true, true, this->config.topology);
}

bool StaticObjectsColorVertexPipeline::initPipeline(const PipelineConfig & config)
{
    try {
        this->config = std::move(dynamic_cast<const StaticColorVertexPipelineConfig &>(config));
    } catch (std::bad_cast ex) {
        logError("StaticObjectsColorVertexPipeline needs instance of StaticColorVertexPipelineConfig!");
        return false;
    }

    for (const auto & s : this->config.shaders) {
        if (!this->addShader((Engine::getAppPath(SHADERS) / s.file).string(), s.shaderType)) {
            logError("Failed to add shader: " + s.file);
        }
    }

    if (this->getNumberOfValidShaders() < 2) {
        logError("StaticObjectsColorVertexPipeline needs vertex and fragment shaders at a minimum!");
        return false;
    }

    if (!this->createBuffers()) {
        logError("Failed to create StaticObjectsColorVertexPipeline buffers");
        return false;
    }

    if (!this->updateBuffers()) {
        logError("Failed to update StaticObjectsColorVertexPipeline buffers");
        return false;
    }

    if (!this->createDescriptorPool()) {
        logError("Failed to create StaticObjectsColorVertexPipeline pipeline Descriptor Pool");
        return false;
    }

    return this->createPipeline();

}

void StaticObjectsColorVertexPipeline::draw(const VkCommandBuffer& commandBuffer, const uint16_t commandBufferIndex)
{
    if (!this->hasPipeline() || !this->isEnabled() || this->config.colorVertices.empty()) return;

    if (this->vertexBuffer.isInitialized()) {
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->layout, 0, 1, &this->descriptors.getDescriptorSets()[commandBufferIndex], 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
    }

    if (this->indexBuffer.isInitialized()) {
        vkCmdBindIndexBuffer(commandBuffer, this->indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    this->correctViewPortCoordinates(commandBuffer);

    if (this->indexBuffer.isInitialized()) {
        vkCmdDrawIndexed(commandBuffer, this->config.indices.size(), 1, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer,this->config.colorVertices.size(), 1, 0, 0);
    }
}

void StaticObjectsColorVertexPipeline::update() {}

bool StaticObjectsColorVertexPipeline::createDescriptors()
{
    if (this->renderer == nullptr || !this->renderer->isReady()) return false;

    this->descriptors.destroy(this->renderer->getLogicalDevice());
    this->descriptorPool.resetPool(this->renderer->getLogicalDevice());

    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);

    this->descriptors.create(this->renderer->getLogicalDevice(), this->descriptorPool.getPool(), this->renderer->getImageCount());

    if (!this->descriptors.isInitialized()) return false;

    const VkDescriptorBufferInfo & ssboBufferVertexInfo = this->vertexBuffer.getDescriptorInfo();

    const uint32_t descSize = this->descriptors.getDescriptorSets().size();
    for (size_t i = 0; i < descSize; i++) {
        const VkDescriptorBufferInfo & uniformBufferInfo = this->renderer->getUniformBuffer(i).getDescriptorInfo();

        int j=0;
        this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, uniformBufferInfo);
        this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, ssboBufferVertexInfo);
    }

    return true;
}

bool StaticObjectsColorVertexPipeline::createDescriptorPool()
{
    if (this->renderer == nullptr || !this->renderer->isReady() || this->descriptorPool.isInitialized()) return false;

    const uint32_t count = this->renderer->getImageCount();

    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);

    this->descriptorPool.createPool(this->renderer->getLogicalDevice(), count);

    return this->descriptorPool.isInitialized();
}


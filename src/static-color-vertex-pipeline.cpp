#include "includes/engine.h"

StaticObjectsColorVertexPipeline::StaticObjectsColorVertexPipeline(const std::string name, Renderer * renderer) : GraphicsPipeline(name, renderer) { }

bool StaticObjectsColorVertexPipeline::createBuffers()
{
    if (this->vertices.empty()) return true;

    // TODO: make more flexible
    // break up into allocation & update
    // better separation for device local and host visible
    this->usesDeviceLocalVertexBuffer = true;
    this->usesDeviceLocalIndexBuffer = true;

    Buffer stagingBuffer;

    if (this->usesDeviceLocalVertexBuffer) this->renderer->trackDeviceLocalMemory(this->vertexBuffer.getSize(), true);
    this->vertexBuffer.destroy(this->renderer->getLogicalDevice());

    VkDeviceSize contentSize = this->vertices.size() * sizeof(ColorVertex);
    VkDeviceSize resevedSize =
        (this->config.reservedVertexSpace > 0 && this->config.reservedVertexSpace > contentSize) ?
            this->config.reservedVertexSpace : contentSize;

    VkResult result = this->vertexBuffer.createDeviceLocalBuffer(
        stagingBuffer, 0, resevedSize,
        this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(),
        this->renderer->getGraphicsCommandPool(), this->renderer->getGraphicsQueue()
    );

    if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
        result = this->vertexBuffer.createSharedStorageBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), resevedSize);
        if (result == VK_SUCCESS) this->usesDeviceLocalVertexBuffer = false;
    }

    if (!this->vertexBuffer.isInitialized()) {
        logError("Failed to create StaticObjectsColorVertexPipeline Vertex Buffer!");
        return false;
    }
    if (this->usesDeviceLocalVertexBuffer) {
        this->renderer->trackDeviceLocalMemory(this->vertexBuffer.getSize());

        stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), contentSize);
        if (stagingBuffer.isInitialized()) {
            stagingBuffer.updateContentSize(contentSize);
            memcpy(static_cast<char *>(stagingBuffer.getBufferData()), this->vertices.data(), stagingBuffer.getContentSize());

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
        memcpy(static_cast<char *>(this->vertexBuffer.getBufferData()), this->vertices.data(), contentSize);
        this->vertexBuffer.updateContentSize(contentSize);
    }

    if (this->indexes.empty()) return true;

    contentSize = this->indexes.size() * sizeof(uint32_t);
    resevedSize =
    (this->config.reservedIndexSpace > 0 && this->config.reservedIndexSpace > contentSize) ?
        this->config.reservedIndexSpace : contentSize;

    if (this->usesDeviceLocalIndexBuffer) this->renderer->trackDeviceLocalMemory(this->indexBuffer.getSize(), true);
    this->indexBuffer.destroy(this->renderer->getLogicalDevice());

    result = this->indexBuffer.createDeviceLocalBuffer(
        stagingBuffer, 0, contentSize,
        this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(),
        this->renderer->getGraphicsCommandPool(), this->renderer->getGraphicsQueue(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );

    if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
        result = this->indexBuffer.createSharedIndexBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), resevedSize);
        if (result == VK_SUCCESS) this->usesDeviceLocalIndexBuffer = false;
    }

    if (!this->indexBuffer.isInitialized()) {
        logError("Failed to create StaticObjectsColorVertexPipeline Index Buffer!");
        return false;
    }
    if (this->usesDeviceLocalIndexBuffer) {
        this->renderer->trackDeviceLocalMemory(this->indexBuffer.getSize());

        stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), contentSize);
        if (stagingBuffer.isInitialized()) {
            stagingBuffer.updateContentSize(contentSize);
            memcpy(static_cast<char *>(stagingBuffer.getBufferData()), this->indexes.data(), stagingBuffer.getContentSize());

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
        memcpy(static_cast<char *>(this->indexBuffer.getBufferData()), this->indexes.data(), contentSize);
        this->indexBuffer.updateContentSize(contentSize);
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

    this->vertices = this->config.colorVertices;

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

    if (!this->createDescriptorPool()) {
        logError("Failed to create StaticObjectsColorVertexPipeline pipeline Descriptor Pool");
        return false;
    }

    return this->createPipeline();

}

void StaticObjectsColorVertexPipeline::draw(const VkCommandBuffer& commandBuffer, const uint16_t commandBufferIndex)
{
    if (!this->hasPipeline() || !this->isEnabled() || this->vertices.empty()) return;

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
        vkCmdDrawIndexed(commandBuffer, this->indexes.size(), 1, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer,this->vertices.size(), 1, 0, 0);
    }
}

void StaticObjectsColorVertexPipeline::update() {}

bool StaticObjectsColorVertexPipeline::createDescriptors()
{
    if (this->renderer == nullptr || !this->renderer->isReady()) return false;

    this->descriptors.destroy(this->renderer->getLogicalDevice());
    this->descriptorPool.resetPool(this->renderer->getLogicalDevice());

    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
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


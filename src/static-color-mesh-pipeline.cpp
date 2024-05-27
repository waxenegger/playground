#include "includes/engine.h"

StaticObjectsColorMeshPipeline::StaticObjectsColorMeshPipeline(const std::string name, Renderer * renderer) : GraphicsPipeline(name, renderer) { }

bool StaticObjectsColorMeshPipeline::createBuffers(const ColorMeshPipelineConfig & conf)
{
    if (conf.reservedVertexSpace == 0) {
        logError("The configuration has reserved 0 space for vertex buffers!");
        return false;
    }

    VkResult result;

    VkDeviceSize reservedSize = conf.reservedVertexSpace;

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
        logError("Failed to create  '" + this->name + "' Pipeline Vertex Buffer!");
        return false;
    }

    reservedSize = conf.reservedIndexSpace;
    if (reservedSize == 0) {
        logInfo("Warning: The configuration has reserved 0 space for index buffers!");
        return true;
    }

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
        logError("Failed to create  '" + this->name + "' Pipeline Index Buffer!");
        return false;
    }

    return true;
}

void StaticObjectsColorMeshPipeline::clearObjectsToBeRenderer() {
    this->objectsToBeRendered.clear();
    if (this->indexBuffer.isInitialized()) this->indexBuffer.updateContentSize(0);
    if (this->vertexBuffer.isInitialized()) this->vertexBuffer.updateContentSize(0);
}

bool StaticObjectsColorMeshPipeline::addObjectsToBeRenderer(const std::vector<StaticColorMeshRenderable *> & additionalObjectsToBeRendered) {
    if (!this->vertexBuffer.isInitialized() || additionalObjectsToBeRendered.empty()) return false;

    /*
    std::vector<ColorVertex> additionalVertices;
    std::vector<uint32_t> additionalIndices;

    const VkDeviceSize vertexBufferContentSize =  this->vertexBuffer.getContentSize();
    const VkDeviceSize indexBufferContentSize =  this->indexBuffer.getContentSize();

    const VkDeviceSize vertexBufferSize = this->vertexBuffer.getSize();
    const VkDeviceSize indexBufferSize = this->indexBuffer.getSize();

    VkDeviceSize vertexBufferAdditionalContentSize =  0;
    VkDeviceSize indexBufferAdditionalContentSize =  0;

    // collect new vertices and indices
    uint32_t additionalObjectsAdded = 0;
    for (const auto & o : additionalObjectsToBeRendered) {
        if (!o->hasBeenRegistered()) {
            logInfo("Warning: Object to be rendered has not been registered with the GlobalRenderableStore!");
        }

        vertexBufferAdditionalContentSize += sizeof(ColorVertex) * o->getVertices().size();
        indexBufferAdditionalContentSize += sizeof(uint32_t) * o->getIndices().size();

        // only continue if we fit into the pre-allocated size
        if ((vertexBufferContentSize + vertexBufferAdditionalContentSize > vertexBufferSize) ||
                (indexBufferContentSize + indexBufferAdditionalContentSize > indexBufferSize))  {
            logError("Can not update buffer since size is too small!");
            break;
        }

        additionalVertices.insert(additionalVertices.end(), o->getVertices().begin(), o->getVertices().end());
        additionalIndices.insert(additionalIndices.end(), o->getIndices().begin(), o->getIndices().end());
        additionalObjectsAdded++;
    }

    if (additionalObjectsAdded == 0) return true;

    if (!this->addObjectsToBeRendererCommon(additionalVertices, additionalIndices)) return false;

    this->objectsToBeRendered.insert(
        this->objectsToBeRendered.end(), additionalObjectsToBeRendered.begin(),
        additionalObjectsToBeRendered.begin() + additionalObjectsAdded
    );

    */
    return true;
}

bool StaticObjectsColorMeshPipeline::addObjectsToBeRendererCommon(const std::vector<VertexMesh> & additionalMeshes) {

    if (!this->vertexBuffer.isInitialized() || additionalMeshes.empty()) return false;

    /*
    const VkDeviceSize vertexBufferContentSize =  this->vertexBuffer.getContentSize();
    VkDeviceSize vertexBufferAdditionalContentSize =  additionalVertices.size() * sizeof(ColorVertex);

    const VkDeviceSize indexBufferContentSize =  this->indexBuffer.getContentSize();
    VkDeviceSize indexBufferAdditionalContentSize =  additionalIndices.size() * sizeof(uint32_t);

    Buffer stagingBuffer;

    if (this->vertexBuffer.isInitialized() && !additionalVertices.empty()) {
        if (this->usesDeviceLocalVertexBuffer) {
            stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), vertexBufferAdditionalContentSize);
            if (stagingBuffer.isInitialized()) {
                stagingBuffer.updateContentSize(vertexBufferAdditionalContentSize);
                memcpy(static_cast<char *>(stagingBuffer.getBufferData()), additionalVertices.data(), stagingBuffer.getContentSize());

                const CommandPool & pool = renderer->getGraphicsCommandPool();
                const VkCommandBuffer & commandBuffer = pool.beginPrimaryCommandBuffer(renderer->getLogicalDevice());

                VkBufferCopy copyRegion {};
                copyRegion.srcOffset = 0;
                copyRegion.dstOffset = vertexBufferContentSize;
                copyRegion.size = vertexBufferAdditionalContentSize;
                vkCmdCopyBuffer(commandBuffer, stagingBuffer.getBuffer(), this->vertexBuffer.getBuffer(), 1, &copyRegion);

                pool.endCommandBuffer(commandBuffer);
                pool.submitCommandBuffer(renderer->getLogicalDevice(), renderer->getGraphicsQueue(), commandBuffer);

                stagingBuffer.destroy(this->renderer->getLogicalDevice());

                this->vertexBuffer.updateContentSize(vertexBufferContentSize + vertexBufferAdditionalContentSize);
            }
        } else {
            memcpy(static_cast<char *>(this->vertexBuffer.getBufferData()) + vertexBufferContentSize, additionalVertices.data(), vertexBufferAdditionalContentSize);
            this->vertexBuffer.updateContentSize(vertexBufferContentSize + vertexBufferAdditionalContentSize);
        }
    }

    if (this->indexBuffer.isInitialized() && !additionalIndices.empty()) {
        if (this->usesDeviceLocalIndexBuffer) {
            stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), indexBufferAdditionalContentSize);
            if (stagingBuffer.isInitialized()) {
                stagingBuffer.updateContentSize(indexBufferAdditionalContentSize);
                memcpy(static_cast<char *>(stagingBuffer.getBufferData()), additionalIndices.data(), stagingBuffer.getContentSize());

                const CommandPool & pool = renderer->getGraphicsCommandPool();
                const VkCommandBuffer & commandBuffer = pool.beginPrimaryCommandBuffer(renderer->getLogicalDevice());

                VkBufferCopy copyRegion {};
                copyRegion.srcOffset = 0;
                copyRegion.dstOffset = indexBufferContentSize;
                copyRegion.size = indexBufferAdditionalContentSize;
                vkCmdCopyBuffer(commandBuffer, stagingBuffer.getBuffer(), this->indexBuffer.getBuffer(), 1, &copyRegion);

                pool.endCommandBuffer(commandBuffer);
                pool.submitCommandBuffer(renderer->getLogicalDevice(), renderer->getGraphicsQueue(), commandBuffer);

                stagingBuffer.destroy(this->renderer->getLogicalDevice());

                this->indexBuffer.updateContentSize(indexBufferContentSize + indexBufferAdditionalContentSize);
            }
        } else {
            memcpy(static_cast<char *>(this->indexBuffer.getBufferData()) + indexBufferContentSize, additionalIndices.data(), indexBufferAdditionalContentSize);
            this->indexBuffer.updateContentSize(indexBufferContentSize + indexBufferAdditionalContentSize);
        }
    }
    */

    return true;
}

bool StaticObjectsColorMeshPipeline::createPipeline()
{
    if (!this->createDescriptors()) {
        logError("Failed to create '" + this->name + "' Pipeline Descriptors");
        return false;
    }

    return this->createGraphicsPipelineCommon(true, true, true, this->config.topology);
}

bool StaticObjectsColorMeshPipeline::initPipeline(const PipelineConfig & config)
{
    try {
        this->config = std::move(dynamic_cast<const StaticObjectsColorMeshPipelineConfig &>(config));
    } catch (std::bad_cast ex) {
        logError("'" + this->name + "' Pipeline needs instance of StaticObjectsColorMeshPipelineConfig!");
        return false;
    }

    for (const auto & s : this->config.shaders) {
        if (!this->addShader((Engine::getAppPath(SHADERS) / s.file).string(), s.shaderType)) {
            logError("Failed to add shader: " + s.file);
        }
    }

    if (this->getNumberOfValidShaders() < 2) {
        logError(" '" + this->name + "' Pipeline needs vertex and fragment shaders at a minimum!");
        return false;
    }

    if (!this->createBuffers(this->config)) {
        logError("Failed to create  '" + this->name + "' Pipeline buffers");
        return false;
    }

    if (!this->createDescriptorPool()) {
        logError("Failed to create  '" + this->name + "' Pipeline pipeline Descriptor Pool");
        return false;
    }

    if (!this->config.objectsToBeRendered.empty()) {
        this->addObjectsToBeRenderer((this->config.objectsToBeRendered));
        this->config.objectsToBeRendered.clear();
    }

    return this->createPipeline();

}

void StaticObjectsColorMeshPipeline::draw(const VkCommandBuffer& commandBuffer, const uint16_t commandBufferIndex)
{
    /*
    if (!this->hasPipeline() || !this->isEnabled() || this->objectsToBeRendered.empty()) return;

    if (this->vertexBuffer.isInitialized()) {
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->layout, 0, 1, &this->descriptors.getDescriptorSets()[commandBufferIndex], 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
    }

    if (this->indexBuffer.isInitialized()) {
        vkCmdBindIndexBuffer(commandBuffer, this->indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    this->correctViewPortCoordinates(commandBuffer);

    VkDeviceSize vertexOffset = 0;
    VkDeviceSize indexOffset = 0;

    for (const auto & o : this->objectsToBeRendered) {
        const VkDeviceSize vertexCount = o->getVertices().size();
        const VkDeviceSize indexCount = o->getIndices().size();

        if (o->shouldBeRendered(Camera::INSTANCE()->getFrustumPlanes())) {
            if (this->indexBuffer.isInitialized() && indexCount > 0) {
                vkCmdDrawIndexed(commandBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
            } else {
                vkCmdDraw(commandBuffer,vertexCount, 1, vertexOffset, 0);
            }
        }

        vertexOffset += vertexCount;
        indexOffset += indexCount;
    }
    */
}

void StaticObjectsColorMeshPipeline::update() {}

bool StaticObjectsColorMeshPipeline::createDescriptors()
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

bool StaticObjectsColorMeshPipeline::createDescriptorPool()
{
    if (this->renderer == nullptr || !this->renderer->isReady() || this->descriptorPool.isInitialized()) return false;

    const uint32_t count = this->renderer->getImageCount();

    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);

    this->descriptorPool.createPool(this->renderer->getLogicalDevice(), count);

    return this->descriptorPool.isInitialized();
}

StaticObjectsColorMeshPipeline::~StaticObjectsColorMeshPipeline()
{
    this->objectsToBeRendered.clear();
}


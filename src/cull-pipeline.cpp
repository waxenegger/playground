#include "includes/engine.h"

CullPipeline::CullPipeline(const std::string name, Renderer * renderer) : ComputePipeline(name, renderer) { }

bool CullPipeline::initPipeline(const PipelineConfig & config) {
    if (this->renderer == nullptr || !this->renderer->isReady()) {
        logError("Pipeline " + this->name + " requires a ready renderer instance!");
        return false;
    }

    this->config = std::move(static_cast<const ComputePipelineConfig &>(config));
    this->usesDeviceLocalComputeBuffer = this->config.useDeviceLocalForComputeSpace && this->renderer->getDeviceMemory().available >= this->config.reservedComputeSpace;

    for (const auto & s : this->config.shaders) {
        if (!this->addShader((Engine::getAppPath(SHADERS) / s.file).string(), s.shaderType)) {
            logError("Failed to add shader: " + s.file);
        }
    }

    if (this->getNumberOfValidShaders() < 1) {
        logError(" '" + this->name + "' Pipeline needs compute shader at a minimum!");
        return false;
    }

    if (!this->createComputeBuffer()) {
        logError("Failed to create Cull Pipeline Components Buffer");
        return false;
    }

    if (!this->createDescriptorPool()) {
        logError("Failed to create Cull Pipeline Descriptor Pool");
        return false;
    }

    return this->createPipeline();
}

void CullPipeline::linkToGraphicsPipeline(std::variant<std::nullptr_t, ColorMeshPipeline *> & pipeline)
{
    this->linkedGraphicsPipeline = pipeline;
}

bool CullPipeline::createPipeline()
{
    if (!this->createDescriptors()) {
        logError("Failed to create Cull Pipeline Descriptors");
        return false;
    }

    return this->createComputePipelineCommon();
}

bool CullPipeline::createDescriptorPool() {
    if (this->renderer == nullptr || !this->renderer->isReady() || this->descriptorPool.isInitialized()) return false;

    const uint32_t count = this->renderer->getImageCount();

    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
    this->descriptorPool.createPool(this->renderer->getLogicalDevice(), count);

    return this->descriptorPool.isInitialized();
}

bool CullPipeline::createDescriptors() {
    if (this->renderer == nullptr || !this->renderer->isReady()) return false;

    this->descriptors.destroy(this->renderer->getLogicalDevice());
    this->descriptorPool.resetPool(this->renderer->getLogicalDevice());

    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
    this->descriptors.create(this->renderer->getLogicalDevice(), this->descriptorPool.getPool(), this->renderer->getImageCount());

    if (!this->descriptors.isInitialized()) return false;

    const VkDescriptorBufferInfo & indirectDrawInfo = this->renderer->getIndirectDrawBuffer().getDescriptorInfo();
    const VkDescriptorBufferInfo & indirectDrawCountInfo = this->renderer->getIndirectDrawCountBuffer().getDescriptorInfo();
    const VkDescriptorBufferInfo & componentsDrawInfo = this->computeBuffer.getDescriptorInfo();

    const uint32_t descSize = this->descriptors.getDescriptorSets().size();
    for (size_t i = 0; i < descSize; i++) {
        const VkDescriptorBufferInfo & uniformBufferInfo = this->renderer->getUniformComputeBuffer(i).getDescriptorInfo();

        this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), 0, i, uniformBufferInfo);
        this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), 1, i, componentsDrawInfo);
        this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), 2, i, indirectDrawInfo);
        this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), 3, i, indirectDrawCountInfo);
    }

    return true;
}

bool CullPipeline::createComputeBuffer()
{
    if (this->renderer == nullptr || !this->renderer->isReady()) return false;

    VkDeviceSize reservedSize = this->config.reservedComputeSpace;
    if (reservedSize == 0) {
        logError("The configuration has reserved 0 space for compute buffers!");
        return false;
    }

    VkResult result;
    uint64_t limit = this->usesDeviceLocalComputeBuffer ?
        this->renderer->getPhysicalDeviceProperty(ALLOCATION_LIMIT) :
         this->renderer->getPhysicalDeviceProperty(STORAGE_BUFFER_LIMIT);

    if (reservedSize > limit) {
        logError("You tried to allocate more in one go than the GPU's allocation/storage buffer limit");
        return false;
    }

    if (this->usesDeviceLocalComputeBuffer) this->renderer->trackDeviceLocalMemory(this->computeBuffer.getSize(), true);
    this->computeBuffer.destroy(this->renderer->getLogicalDevice());

    if (this->usesDeviceLocalComputeBuffer) {
        result = this->computeBuffer.createDeviceLocalBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);

        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            this->usesDeviceLocalComputeBuffer = false;
        } else {
            this->renderer->trackDeviceLocalMemory(this->computeBuffer.getSize());
        }
    }

    if (!this->usesDeviceLocalComputeBuffer) {
        result = this->computeBuffer.createSharedStorageBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);
    }

    if (!this->computeBuffer.isInitialized()) {
        logError("Failed to create  '" + this->name + "' Pipeline Compute Buffer!");
        return false;
    }

    return true;
}

void CullPipeline::updateWithColorMeshData(ColorMeshPipeline * pipeline) {
    const VkDeviceSize drawCommandSize = sizeof(struct ColorMeshDrawCommand);
    const VkDeviceSize maxSize = this->computeBuffer.getSize();

    const auto & renderables = pipeline->getRenderables();
    if (renderables.size() <= this->instanceOffset) return;

    std::vector<ColorMeshDrawCommand> drawCommands;
    VkDeviceSize computeBufferContentSize = this->computeBuffer.getContentSize();
    VkDeviceSize additionalDrawCommandSize = 0;

    for (uint32_t i=this->instanceOffset;i<renderables.size();i++) {
        auto renderable = renderables[i];

        const VkDeviceSize additionalSize = renderable->getMeshes().size() * drawCommandSize;
        if (computeBufferContentSize + additionalDrawCommandSize + additionalSize > maxSize) {
            logError("Compute Buffer not big enough!");
            break;
        }

        const auto & bbox = renderable->getBoundingBox();

        for (auto & m : renderable->getMeshes()) {
            const ColorMeshDrawCommand drawCommand = {
                static_cast<uint32_t>(m->indices.size()), this->indexOffset, static_cast<int32_t>(this->vertexOffset),
                this->instanceOffset, this->meshOffset, bbox.center, bbox.radius
            };
            drawCommands.emplace_back(drawCommand);

            this->vertexOffset += m->vertices.size();
            this->indexOffset += m->indices.size();
            this->meshOffset++;
        }

        additionalDrawCommandSize += additionalSize;
        this->instanceOffset++;
    }

    if (this->usesDeviceLocalComputeBuffer) {
        Buffer stagingBuffer;
        stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), additionalDrawCommandSize);
        if (stagingBuffer.isInitialized()) {
            stagingBuffer.updateContentSize(additionalDrawCommandSize);

            memcpy(static_cast<char *>(stagingBuffer.getBufferData()), drawCommands.data(), additionalDrawCommandSize);

            const CommandPool & pool = renderer->getGraphicsCommandPool();
            const VkCommandBuffer & commandBuffer = pool.beginPrimaryCommandBuffer(renderer->getLogicalDevice());

            VkBufferCopy copyRegion {};
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = computeBufferContentSize;
            copyRegion.size = additionalDrawCommandSize;
            vkCmdCopyBuffer(commandBuffer, stagingBuffer.getBuffer(), this->computeBuffer.getBuffer(), 1, &copyRegion);

            pool.endCommandBuffer(commandBuffer);
            pool.submitCommandBuffer(renderer->getLogicalDevice(), renderer->getComputeQueue(), commandBuffer);

            stagingBuffer.destroy(this->renderer->getLogicalDevice());
        }
    } else {
        memcpy(static_cast<char *>(this->computeBuffer.getBufferData()) + computeBufferContentSize, drawCommands.data(), additionalDrawCommandSize);
    }

    this->drawCount = this->instanceOffset;
    this->computeBuffer.updateContentSize(computeBufferContentSize + additionalDrawCommandSize);
    this->renderer->setMaxIndirectCallCount(this->drawCount);
}

void CullPipeline::update() {
    if (this->renderer == nullptr || !this->renderer->isReady() || !this->computeBuffer.isInitialized()) return;

    std::visit([this](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ColorMeshPipeline *>) {
            this->updateWithColorMeshData(arg);
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
            logError("Cull Pipeline needs a linked Graphics Pipleline for Data");
        }
    }, this->linkedGraphicsPipeline);
}

void CullPipeline::compute(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex) {

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->layout, 0, 1, &this->descriptors.getDescriptorSets()[commandBufferIndex], 0, nullptr);

    vkCmdDispatch(commandBuffer,(this->drawCount / 32)+1, 1, 1);
}

CullPipeline::~CullPipeline() {
    this->computeBuffer.destroy(this->renderer->getLogicalDevice());
}


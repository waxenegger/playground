#include "includes/engine.h"

CullPipeline::CullPipeline(const std::string name, Renderer * renderer) : ComputePipeline(name, renderer) { }

bool CullPipeline::initPipeline(const PipelineConfig & config) {
    if (this->renderer == nullptr || !this->renderer->isReady()) {
        logError("Pipeline " + this->name + " requires a ready renderer instance!");
        return false;
    }

    this->config = std::move(static_cast<const ComputePipelineConfig &>(config));

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

    if (this->config.reservedComputeSpace == 0) {
        logError("The configuration has reserved 0 space for compute buffers!");
        return false;
    }

    VkResult result;
    VkDeviceSize reservedSize = this->config.reservedComputeSpace;
    this->usesDeviceLocalComputeBuffer = this->renderer->getDeviceMemory().available >= reservedSize;

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
        logError("Failed to create  '" + this->name + "' Pipeline Commpute Buffer!");
        return false;
    }

    return true;
}

void CullPipeline::update() {
    if (this->renderer == nullptr || !this->renderer->isReady() || !this->computeBuffer.isInitialized()) return;

    VkDeviceSize overallSize = 0;
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;

    const VkDeviceSize renderablesBufferSize = sizeof(struct ObjectsDrawCommand);
    const VkDeviceSize maxSize = this->computeBuffer.getSize();

    const auto & renderables = GlobalRenderableStore::INSTANCE()->getRenderables();

    for (auto & r : renderables) {
        auto rStatic = (static_cast<ColorMeshRenderable *>(r.get()));


    }
    /*

    void * data = this->componentsBuffer.getBufferData();

    this->drawCount = 0;

    auto & allModels = Models::INSTANCE()->getModels();
    for (auto & model :  allModels) {
        auto allComponents = Components::INSTANCE()->getAllComponentsForModel(model->getId());

        if (allComponents.empty()) {
            jointDataOffset += allComponents.size() * model->getJointCount();
            continue;
        }

        auto meshes = model->getMeshes();
        uint32_t tmpVertexOffset = 0;

        for (Mesh & mesh : meshes) {
            uint32_t instCount = 0;
            for (auto & c : allComponents) {
                if (overallSize + componentsBufferSize > maxSize) {
                    this->drawCount = overallSize / componentsBufferSize;
                    break;
                }

                if (c->isVisible()) {
                    const BoundingBox bbox = c->getModel()->getBoundingBox();
                    const glm::mat4 matrix = c->getModelMatrix();
                    const ComponentsDrawCommand componentsDrawCommand = {
                        mesh.getIndexCount(),
                        mesh.getIndexOffset(),
                        mesh.getVertexOffset(),
                        vertexOffset,
                        instanceDataOffset+instCount,
                        meshDataOffset,
                        model->hasAnimations() ? static_cast<int32_t>(jointDataOffset + instCount * model->getVertexJointInfoCount()) : -1,
                        glm::vec3(matrix * glm::vec4(bbox.center, 1)),
                        bbox.radius * c->getScalingFactor()
                    };

                    memcpy(static_cast<char *>(data)+overallSize, &componentsDrawCommand, componentsBufferSize);
                    overallSize += componentsBufferSize;
                }

                instCount++;
                meshDataOffset++;
            }
            tmpVertexOffset += mesh.getVertexCount();
        }

        vertexOffset += tmpVertexOffset;
        instanceDataOffset += allComponents.size();

        if (model->hasAnimations()) {
            jointDataOffset +=   allComponents.size() * model->getVertexJointInfoCount();
        }
    }

    this->drawCount = overallSize / componentsBufferSize;
    this->renderablesBuffer.updateContentSize(overallSize);
    this->renderer->setMaxIndirectCallCount(this->drawCount);
    */
}

void CullPipeline::compute(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex) {

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->layout, 0, 1, &this->descriptors.getDescriptorSets()[commandBufferIndex], 0, nullptr);

    vkCmdDispatch(commandBuffer,(this->drawCount / 32)+1, 1, 1);
}

CullPipeline::~CullPipeline() {
    this->computeBuffer.destroy(this->renderer->getLogicalDevice());
}


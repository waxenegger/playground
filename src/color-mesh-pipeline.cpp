#include "includes/engine.h"

ColorMeshPipeline::ColorMeshPipeline(const std::string name, Renderer * renderer) : GraphicsPipeline(name, renderer) { }

bool ColorMeshPipeline::createBuffers(const ColorMeshPipelineConfig & conf)
{
    /**
     *  VERTEX BUFFER CREATION
     */

    if (conf.reservedVertexSpace == 0) {
        logError("The configuration has reserved 0 space for vertex buffers!");
        return false;
    }

    VkResult result;
    VkDeviceSize reservedSize = conf.reservedVertexSpace;

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

    /**
     *  INDEX BUFFER CREATION
     */

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

    /**
     *  STORAGE BUFFER CREATION FOR INSTANCE AND MESH DATA
     *  ONLY FOR COMPUTE CULLING + INDIRECT GPU DRAW
     */

    if (USE_GPU_CULLING) {
        reservedSize = conf.reservedInstanceDataSpace;

        limit = this->renderer->getPhysicalDeviceProperty(STORAGE_BUFFER_LIMIT);
        if (reservedSize > limit) {
            logError("You tried to allocate more in one go than the GPU's allocation/storage buffer limit");
            return false;
        }

        this->ssboInstanceBuffer.destroy(this->renderer->getLogicalDevice());
        this->ssboInstanceBuffer.createSharedStorageBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);
        if (!this->ssboInstanceBuffer.isInitialized()) {
            logError("Failed to create  '" + this->name + "' Pipeline SSBO Instance Buffer!");
            return false;
        }

        reservedSize = conf.reservedMeshDataSpace;

        limit = this->renderer->getPhysicalDeviceProperty(STORAGE_BUFFER_LIMIT);
        if (reservedSize > limit) {
            logError("You tried to allocate more in one go than the GPU's allocation/storage buffer limit");
            return false;
        }

        this->ssboMeshBuffer.destroy(this->renderer->getLogicalDevice());
        this->ssboMeshBuffer.createSharedStorageBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);
        if (!this->ssboMeshBuffer.isInitialized()) {
            logError("Failed to create  '" + this->name + "' Pipeline SSBO Mesh Data Buffer!");
            return false;
        }
    }

    return true;
}

bool ColorMeshPipeline::createPipeline()
{
    if (!this->createDescriptors()) {
        logError("Failed to create '" + this->name + "' Pipeline Descriptors");
        return false;
    }

    return this->createGraphicsPipelineCommon(true, true, true, this->config.topology);
}

bool ColorMeshPipeline::initPipeline(const PipelineConfig & config)
{
    if (this->renderer == nullptr || !this->renderer->isReady()) {
        logError("Pipeline " + this->name + " requires a ready renderer instance!");
        return false;
    }

    // set some handed in parameters from the configuration
    // mainly for device local and storage buffers
    // and whether which one should be used

    this->config = std::move(static_cast<const ColorMeshPipelineConfig &>(config));
    this->usesDeviceLocalVertexBuffer = this->config.useDeviceLocalForVertexSpace && this->renderer->getDeviceMemory().available >= this->config.reservedVertexSpace;
    this->usesDeviceLocalIndexBuffer = this->config.useDeviceLocalForIndexSpace && this->renderer->getDeviceMemory().available >= this->config.reservedIndexSpace;

    this->pushConstantRange = VkPushConstantRange {};

    if (!USE_GPU_CULLING) {
        this->pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        this->pushConstantRange.offset = 0;
        this->pushConstantRange.size = sizeof(ColorMeshPushConstants);
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

    // add any handed in objects to the pipeline for drawing
    if (!this->config.objectsToBeRendered.empty()) {
        this->addObjectsToBeRenderer(this->config.objectsToBeRendered);
        this->config.objectsToBeRendered.clear();
    }

    return this->createPipeline();
}

void ColorMeshPipeline::clearObjectsToBeRenderer() {
    this->objectsToBeRendered.clear();
    if (this->indexBuffer.isInitialized()) this->indexBuffer.updateContentSize(0);
    if (this->vertexBuffer.isInitialized()) this->vertexBuffer.updateContentSize(0);
}

bool ColorMeshPipeline::addObjectsToBeRendererCommon(const std::vector<Vertex *> & additionalVertices, const std::vector<uint32_t *> & additionalIndices) {

    if (!this->vertexBuffer.isInitialized() || additionalVertices.empty()) return false;

    const VkDeviceSize vertexBufferContentSize =  this->vertexBuffer.getContentSize();
    VkDeviceSize vertexBufferAdditionalContentSize =  additionalVertices.size() * sizeof(Vertex);

    const VkDeviceSize indexBufferContentSize =  this->indexBuffer.getContentSize();
    VkDeviceSize indexBufferAdditionalContentSize =  additionalIndices.size() * sizeof(uint32_t);

    Buffer stagingBuffer;
    VkDeviceSize offset = 0;
    VkDeviceSize step = 0;

    if (this->vertexBuffer.isInitialized() && !additionalVertices.empty()) {
        if (this->usesDeviceLocalVertexBuffer) {
            stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), vertexBufferAdditionalContentSize);
            if (stagingBuffer.isInitialized()) {
                stagingBuffer.updateContentSize(vertexBufferAdditionalContentSize);

                offset = 0;
                step = sizeof(Vertex);
                for (const auto vert : additionalVertices) {
                    memcpy(static_cast<char *>(stagingBuffer.getBufferData())+offset, vert, step);
                    offset += step;
                }

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
            offset = 0;
            step = sizeof(Vertex);
            for (const auto vert : additionalVertices) {
                memcpy(static_cast<char *>(this->vertexBuffer.getBufferData()) + vertexBufferContentSize + offset, vert, step);
                offset += step;
            }
            this->vertexBuffer.updateContentSize(vertexBufferContentSize + vertexBufferAdditionalContentSize);
        }
    }

    if (this->indexBuffer.isInitialized() && !additionalIndices.empty()) {
        if (this->usesDeviceLocalIndexBuffer) {
            stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), indexBufferAdditionalContentSize);
            if (stagingBuffer.isInitialized()) {
                stagingBuffer.updateContentSize(indexBufferAdditionalContentSize);

                offset = 0;
                step = sizeof(uint32_t);
                for (const auto ind : additionalIndices) {
                    memcpy(static_cast<char *>(stagingBuffer.getBufferData())+offset, ind, step);
                    offset += step;
                }

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
            offset = 0;
            step = sizeof(uint32_t);
            for (const auto ind : additionalIndices) {
                memcpy(static_cast<char *>(this->indexBuffer.getBufferData()) + indexBufferContentSize + offset, ind, step);
                offset += step;
            }
            this->indexBuffer.updateContentSize(indexBufferContentSize + indexBufferAdditionalContentSize);
        }
    }

    return true;
}

bool ColorMeshPipeline::addObjectsToBeRenderer(const std::vector<ColorMeshRenderable *> & additionalObjectsToBeRendered) {
    if (!this->vertexBuffer.isInitialized() || additionalObjectsToBeRendered.empty()) return false;

    std::vector<Vertex *> additionalVertices;
    std::vector<uint32_t *> additionalIndices;

    const VkDeviceSize vertexBufferContentSize =  this->vertexBuffer.getContentSize();
    const VkDeviceSize indexBufferContentSize =  this->indexBuffer.getContentSize();
    const VkDeviceSize meshDataBufferContentSize = this->ssboMeshBuffer.getContentSize();

    const VkDeviceSize vertexBufferSize = this->vertexBuffer.getSize();
    const VkDeviceSize indexBufferSize = this->indexBuffer.getSize();
    const VkDeviceSize meshDataBufferSize = this->ssboMeshBuffer.getSize();

    const VkDeviceSize meshDataSize = sizeof(ColorMeshData);
    std::vector<ColorMeshData> meshDatas;

    VkDeviceSize vertexBufferAdditionalContentSize =  0;
    VkDeviceSize indexBufferAdditionalContentSize =  0;
    VkDeviceSize meshDataBufferAdditionalContentSize =  0;


    std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();

    // collect new vertices and indices
    uint32_t additionalObjectsAdded = 0;
    for (const auto & o : additionalObjectsToBeRendered) {
        if (!o->hasBeenRegistered()) {
            logInfo("Warning: Object to be rendered has not been registered with the GlobalRenderableStore!");
        }

        bool bufferTooSmall = false;
        for (const auto & mesh : o->getMeshes()) {
            vertexBufferAdditionalContentSize += sizeof(Vertex) * mesh->vertices.size();
            indexBufferAdditionalContentSize += sizeof(uint32_t) * mesh->indices.size();
            meshDataBufferAdditionalContentSize += meshDataSize;

            // only continue if we fit into the pre-allocated size
            if ((vertexBufferContentSize + vertexBufferAdditionalContentSize > vertexBufferSize) ||
                    (indexBufferContentSize + indexBufferAdditionalContentSize > indexBufferSize) ||
                    (meshDataBufferContentSize + meshDataBufferAdditionalContentSize > meshDataBufferSize))  {
                logError("Can not update buffer since size is too small!");
                bufferTooSmall = true;
                break;
            }

            const ColorMeshData meshData = { mesh->color };
            meshDatas.emplace_back(meshData);

            additionalVertices.insert(additionalVertices.end(), mesh->vertices.begin(), mesh->vertices.end());
            additionalIndices.insert(additionalIndices.end(), mesh->indices.begin(), mesh->indices.end());
        }

        if (bufferTooSmall) break;

        additionalObjectsAdded++;
    }

    const VkDeviceSize totalMeshDataSize =  meshDataSize * meshDatas.size();
    memcpy(static_cast<char *>(this->ssboMeshBuffer.getBufferData())+ meshDataBufferContentSize, meshDatas.data(), totalMeshDataSize);
    this->ssboMeshBuffer.updateContentSize(meshDataBufferContentSize + totalMeshDataSize);


    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - begin;
    logInfo("Second Loop: " + std::to_string(time_span.count()));

    if (additionalObjectsAdded == 0) return true;

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    // delegate filling up vertex and index buffeers to break up code
    if (!this->addObjectsToBeRendererCommon(additionalVertices, additionalIndices)) return false;

    time_span = std::chrono::high_resolution_clock::now() - start;
    logInfo("Common: " + std::to_string(time_span.count()));

    /**
     * Populate per instance and mesh data buffers
     * only used for compute culling + indirect GPU draw
     */
    if (USE_GPU_CULLING) {
        uint32_t c=0;
        VkDeviceSize instanceDataOffset = this->ssboInstanceBuffer.getContentSize();
        VkDeviceSize instanceDataBufferSize = this->ssboInstanceBuffer.getSize();

        const uint32_t instanceDataSize = sizeof(ColorMeshInstanceData);
        std::vector<ColorMeshInstanceData> meshInstanceData;

        for (auto & o : additionalObjectsToBeRendered) {
            if ((instanceDataOffset + c * instanceDataSize > instanceDataBufferSize) || (c >= additionalObjectsAdded)) break;

            const ColorMeshInstanceData instanceData = { o->getMatrix() };
            meshInstanceData.emplace_back(instanceData);

            c++;
        }
        additionalObjectsAdded = c;

        const VkDeviceSize totalIntanceDataSize = instanceDataSize * meshInstanceData.size();
        memcpy(static_cast<char *>(this->ssboInstanceBuffer.getBufferData()) + instanceDataOffset, meshInstanceData.data(), totalIntanceDataSize);

        this->ssboInstanceBuffer.updateContentSize(instanceDataOffset + totalIntanceDataSize);
    }


    // finally add object references to be used for draw loop and object updates
    this->objectsToBeRendered.insert(
        this->objectsToBeRendered.end(), additionalObjectsToBeRendered.begin(),
        additionalObjectsToBeRendered.begin() + additionalObjectsAdded
    );

    return true;
}

void ColorMeshPipeline::draw(const VkCommandBuffer& commandBuffer, const uint16_t commandBufferIndex)
{
    if (!this->hasPipeline() || !this->isEnabled() || this->objectsToBeRendered.empty()) return;

    // common descriptor set and pipeline bindings

    if (this->vertexBuffer.isInitialized()) {
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->layout, 0, 1, &this->descriptors.getDescriptorSets()[commandBufferIndex], 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
    }

    if (this->indexBuffer.isInitialized()) {
        vkCmdBindIndexBuffer(commandBuffer, this->indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    this->correctViewPortCoordinates(commandBuffer);


    /**
     * The draw is either done indirectly (using the compute shader frustum culling (USE_GPU_CULLING == true)
     * in which case the per instance and mesh data is in respective storage buffers
     * OR
     * we draw directly handing in matrix and mesh color via push ColorMeshPushConstants
     *
     * Note: The former method is default and outperforms the latter
     */

    if (USE_GPU_CULLING) {
        const VkDeviceSize indirectDrawBufferSize = sizeof(struct ColorMeshIndirectDrawCommand);

        uint32_t maxSize = this->renderer->getMaxIndirectCallCount();

        const VkBuffer & buffer = this->renderer->getIndirectDrawBuffer().getBuffer();
        const VkBuffer & countBuffer = this->renderer->getIndirectDrawCountBuffer().getBuffer();

        if (this->indexBuffer.isInitialized()) {
            vkCmdDrawIndexedIndirectCount(commandBuffer, buffer,0, countBuffer, 0, maxSize, indirectDrawBufferSize);
        } else {
            vkCmdDrawIndirectCount(commandBuffer, buffer,0, countBuffer, 0, maxSize, indirectDrawBufferSize);
        }

        return;
    }

    // direct drawing from here on only

    VkDeviceSize vertexOffset = 0;
    VkDeviceSize indexOffset = 0;

    for (const auto & o : this->objectsToBeRendered) {
        for (const auto & m : o->getMeshes()) {
            const VkDeviceSize vertexCount = m->vertices.size();
            const VkDeviceSize indexCount = m->indices.size();

            if (o->shouldBeRendered(Camera::INSTANCE()->getFrustumPlanes())) {
                const ColorMeshPushConstants & pushConstants = { o->getMatrix(), m->color };
                vkCmdPushConstants(commandBuffer, this->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants) , &pushConstants);

                if (this->indexBuffer.isInitialized() && indexCount > 0) {
                    vkCmdDrawIndexed(commandBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
                } else {
                    vkCmdDraw(commandBuffer,vertexCount, 1, vertexOffset, 0);
                }
            }

            vertexOffset += vertexCount;
            indexOffset += indexCount;
        }
    }
}

void ColorMeshPipeline::update() {}

bool ColorMeshPipeline::createDescriptors()
{
    if (this->renderer == nullptr || !this->renderer->isReady()) return false;

    this->descriptors.destroy(this->renderer->getLogicalDevice());
    this->descriptorPool.resetPool(this->renderer->getLogicalDevice());

    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);

    if (USE_GPU_CULLING) {
        this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
        this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
        this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
    }

    this->descriptors.create(this->renderer->getLogicalDevice(), this->descriptorPool.getPool(), this->renderer->getImageCount());

    if (!this->descriptors.isInitialized()) return false;

    const VkDescriptorBufferInfo & ssboBufferVertexInfo = this->vertexBuffer.getDescriptorInfo();

    const VkDescriptorBufferInfo & indirectDrawInfo = this->renderer->getIndirectDrawBuffer().getDescriptorInfo();
    const VkDescriptorBufferInfo & ssboInstanceBufferInfo = this->ssboInstanceBuffer.getDescriptorInfo();
    const VkDescriptorBufferInfo & ssboMeshDataBufferInfo = this->ssboMeshBuffer.getDescriptorInfo();

    const uint32_t descSize = this->descriptors.getDescriptorSets().size();
    for (size_t i = 0; i < descSize; i++) {
        const VkDescriptorBufferInfo & uniformBufferInfo = this->renderer->getUniformBuffer(i).getDescriptorInfo();

        int j=0;
        this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, uniformBufferInfo);
        this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, ssboBufferVertexInfo);

        if (USE_GPU_CULLING) {
            this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, indirectDrawInfo);
            this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, ssboInstanceBufferInfo);
            this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, ssboMeshDataBufferInfo);
        }
    }

    return true;
}

bool ColorMeshPipeline::createDescriptorPool()
{
    if (this->renderer == nullptr || !this->renderer->isReady() || this->descriptorPool.isInitialized()) return false;

    const uint32_t count = this->renderer->getImageCount();

    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
    if (USE_GPU_CULLING) {
        this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
        this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
        this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
    }

    this->descriptorPool.createPool(this->renderer->getLogicalDevice(), count);

    return this->descriptorPool.isInitialized();
}

ColorMeshPipeline::~ColorMeshPipeline()
{
    this->objectsToBeRendered.clear();
}



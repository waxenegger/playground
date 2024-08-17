#include "includes/engine.h"

template<>
bool VertexMeshPipeline0::needsImageSampler() {
    return false;
}

template<>
bool VertexMeshPipeline0::needsAnimationMatrices() {
    return false;
}

template<>
bool VertexMeshPipeline0::initPipeline(const PipelineConfig & config)
{
    if (this->renderer == nullptr || !this->renderer->isReady()) {
        logError("Pipeline " + this->name + " requires a ready renderer instance!");
        return false;
    }

    // set some handed in parameters from the configuration
    // mainly for device local and storage buffers
    // and whether which one should be used

    this->config = std::move(static_cast<const VertexMeshPipelineConfig &>(config));
    this->usesDeviceLocalVertexBuffer = this->config.useDeviceLocalForVertexSpace && this->renderer->getDeviceMemory().available >= this->config.reservedVertexSpace;
    this->usesDeviceLocalIndexBuffer = this->config.useDeviceLocalForIndexSpace && this->renderer->getDeviceMemory().available >= this->config.reservedIndexSpace;

    if (this->renderer->usesGpuCulling()) {
        if (this->config.indirectBufferIndex < 0) {
            logError("Pipeline " + this->name + " requires an indirect buffer index for GPU culling");
            return false;
        }
        this->indirectBufferIndex = this->config.indirectBufferIndex;
    }

    this->pushConstantRange = VkPushConstantRange {};

    if (!this->renderer->usesGpuCulling()) {
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

    // delegate
    VertexMeshPipelineConfig conf { this->renderer->usesGpuCulling()};
    conf.reservedVertexSpace = this->config.reservedVertexSpace;
    conf.reservedInstanceDataSpace = this->config.reservedInstanceDataSpace;
    conf.reservedMeshDataSpace = this->config.reservedMeshDataSpace;

    if (!this->createBuffers(conf, true)) {
        logError("Failed to create  '" + this->name + "' Pipeline buffers");
        return false;
    }

    if (!this->createDescriptorPool()) {
        logError("Failed to create  '" + this->name + "' Pipeline pipeline Descriptor Pool");
        return false;
    }

    // add any handed in objects to the pipeline for drawing
    if (!this->config.objectsToBeRendered.empty()) {
        this->addObjectsToBeRendered(this->config.objectsToBeRendered);
        this->config.objectsToBeRendered.clear();
    }

    return this->createPipeline();
}

template<>
bool VertexMeshPipeline0::addObjectsToBeRendered(const std::vector<VertexMeshRenderable *> & additionalObjectsToBeRendered, const bool useAltGraphicsQueue) {
    if (!this->vertexBuffer.isInitialized() || additionalObjectsToBeRendered.empty()) return false;

    const std::lock_guard<std::mutex> lock(this->additionMutex);

    std::vector<Vertex> additionalVertices;
    std::vector<uint32_t> additionalIndices;

    VkDeviceSize vertexBufferContentSize =  this->vertexBuffer.getContentSize();
    const VkDeviceSize meshDataBufferContentSize = this->ssboMeshBuffer.getContentSize();

    const VkDeviceSize vertexBufferSize = this->vertexBuffer.getSize();
    const VkDeviceSize meshDataBufferSize = this->ssboMeshBuffer.getSize();

    const VkDeviceSize meshDataSize = sizeof(ColorMeshData);
    std::vector<ColorMeshData> meshDatas;

    VkDeviceSize vertexBufferAdditionalContentSize =  0;
    VkDeviceSize meshDataBufferAdditionalContentSize =  0;

    // collect new vertices
    uint32_t additionalObjectsAdded = 0;
    for (const auto & o : additionalObjectsToBeRendered) {
        if (!o->hasBeenRegistered()) {
            logInfo("Warning: Object to be rendered has not been registered with the GlobalRenderableStore!");
        }

        bool bufferTooSmall = false;
        for (const auto & mesh : o->getMeshes()) {
            vertexBufferAdditionalContentSize += sizeof(Vertex) * mesh.vertices.size();
            meshDataBufferAdditionalContentSize += meshDataSize;

            // only continue if we fit into the pre-allocated size
            if ((vertexBufferContentSize + vertexBufferAdditionalContentSize > vertexBufferSize) ||
                    (this->renderer->usesGpuCulling() && (meshDataBufferContentSize + meshDataBufferAdditionalContentSize > meshDataBufferSize)))  {
                logError("Pipeline '" + this->name + "': buffer size too small. Added " + std::to_string(additionalObjectsAdded) + " of " + std::to_string(additionalObjectsToBeRendered.size()));
                bufferTooSmall = true;
                break;
            }

            if (this->renderer->usesGpuCulling()) {
                const ColorMeshData meshData = { mesh.color };
                meshDatas.emplace_back(meshData);
            }

            additionalVertices.insert(additionalVertices.end(), mesh.vertices.begin(), mesh.vertices.end());
        }

        if (bufferTooSmall) break;

        // do this in batches
        if (vertexBufferAdditionalContentSize > 250 * MEGA_BYTE) {
            if (!this->addObjectsToBeRenderedCommon(additionalVertices.data(), vertexBufferAdditionalContentSize, additionalIndices, useAltGraphicsQueue)) break;

            vertexBufferContentSize = this->vertexBuffer.getContentSize();
            vertexBufferAdditionalContentSize = 0;

            additionalVertices.clear();
        }

        additionalObjectsAdded++;
    }

    if (additionalObjectsAdded == 0) return true;

    if (this->renderer->usesGpuCulling()) {
        const VkDeviceSize totalMeshDataSize =  meshDataSize * meshDatas.size();
        memcpy(static_cast<char *>(this->ssboMeshBuffer.getBufferData())+ meshDataBufferContentSize, meshDatas.data(), totalMeshDataSize);
        this->ssboMeshBuffer.updateContentSize(meshDataBufferContentSize + totalMeshDataSize);
    }

    // delegate filling up vertex and index buffeers to break up code
    if (!this->addObjectsToBeRenderedCommon(additionalVertices.data(), vertexBufferAdditionalContentSize, additionalIndices, useAltGraphicsQueue)) return false;

    /**
     * Populate per instance and mesh data buffers
     * only used for compute culling + indirect GPU draw
     */
    if (this->renderer->usesGpuCulling()) {
        uint32_t c=0;
        VkDeviceSize instanceDataOffset = this->ssboInstanceBuffer.getContentSize();
        VkDeviceSize instanceDataBufferSize = this->ssboInstanceBuffer.getSize();

        const uint32_t instanceDataSize = sizeof(ColorMeshInstanceData);
        std::vector<ColorMeshInstanceData> meshInstanceData;

        for (auto & o : additionalObjectsToBeRendered) {
            if ((instanceDataOffset + c * instanceDataSize > instanceDataBufferSize) || (c >= additionalObjectsAdded)) break;

            const BoundingSphere sphere = o->getBoundingSphere();
            const ColorMeshInstanceData instanceData = {
                o->getMatrix(), sphere.center, sphere.radius
            };
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

template<>
void VertexMeshPipeline0::draw(const VkCommandBuffer& commandBuffer, const uint16_t commandBufferIndex)
{
    if (!this->hasPipeline() || !this->isEnabled() || this->objectsToBeRendered.empty() ||
        !this->vertexBuffer.isInitialized() || this->vertexBuffer.getContentSize() == 0) return;

    // common descriptor set and pipeline bindings
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->layout, 0, 1, &this->descriptors.getDescriptorSets()[commandBufferIndex], 0, nullptr);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

    this->correctViewPortCoordinates(commandBuffer);

    /**
     * The draw is either done indirectly (using the compute shader frustum culling (USE_GPU_CULLING == true)
     * in which case the per instance and mesh data is in respective storage buffers
     * OR
     * we draw directly handing in matrix and mesh color via push ColorMeshPushConstants
     *
     * Note: The former method is default and outperforms the latter
     */

    if (this->renderer->usesGpuCulling()) {
        const VkDeviceSize indirectDrawBufferSize = sizeof(struct VertexMeshIndirectDrawCommand);

        uint32_t maxSize = this->renderer->getMaxIndirectCallCount(this->indirectBufferIndex);

        const VkBuffer & buffer = this->renderer->getIndirectDrawBuffer(this->indirectBufferIndex).getBuffer();
        const VkBuffer & countBuffer = this->renderer->getIndirectDrawCountBuffer(this->indirectBufferIndex).getBuffer();

        vkCmdDrawIndirectCount(commandBuffer, buffer,0, countBuffer, 0, maxSize, indirectDrawBufferSize);

        return;
    }

    // direct drawing from here on only
    VkDeviceSize vertexOffset = 0;
    for (const auto & o : this->objectsToBeRendered) {
        for (const auto & m : o->getMeshes()) {
            const VkDeviceSize vertexCount = m.vertices.size();

            if (o->shouldBeRendered()) {
                const ColorMeshPushConstants & pushConstants = { o->getMatrix(), m.color };
                vkCmdPushConstants(commandBuffer, this->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants) , &pushConstants);
                vkCmdDraw(commandBuffer,vertexCount, 1, vertexOffset, 0);
            }

            vertexOffset += vertexCount;
        }
    }
}

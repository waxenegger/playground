#include "includes/engine.h"

DynamicObjectsColorMeshPipeline::DynamicObjectsColorMeshPipeline(const std::string name, Renderer * renderer) : StaticObjectsColorMeshPipeline(name, renderer) { }

bool DynamicObjectsColorMeshPipeline::createPipeline()
{
    if (!this->createDescriptors()) {
        logError("Failed to create '" + this->name + "' Pipeline Descriptors");
        return false;
    }

    return this->createGraphicsPipelineCommon(true, true, true, this->config.topology);
}

bool DynamicObjectsColorMeshPipeline::initPipeline(const PipelineConfig & config)
{
    try {
        this->config = std::move(dynamic_cast<const DynamicObjectsColorMeshPipelineConfig &>(config));
    } catch (std::bad_cast ex) {
        logError("'" + this->name + "' Pipeline needs instance of DynamicColorMeshPipelineConfig!");
        return false;
    }

    this->pushConstantRange = VkPushConstantRange {};
    this->pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    this->pushConstantRange.offset = 0;
    this->pushConstantRange.size = sizeof(DynamicColorMeshPushConstants);

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
        this->addObjectsToBeRenderer(this->config.objectsToBeRendered);
        this->config.objectsToBeRendered.clear();
    }

    return this->createPipeline();
}

bool DynamicObjectsColorMeshPipeline::addObjectsToBeRenderer(const std::vector<DynamicColorMeshRenderable *> & additionalObjectsToBeRendered) {
    if (!this->vertexBuffer.isInitialized() || additionalObjectsToBeRendered.empty()) return false;

    std::vector<Vertex> additionalVertices;
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

        bool bufferTooSmall = false;
        for (const auto & mesh : o->getMeshes()) {
            vertexBufferAdditionalContentSize += sizeof(Vertex) * mesh.vertices.size();
            indexBufferAdditionalContentSize += sizeof(uint32_t) * mesh.indices.size();

            // only continue if we fit into the pre-allocated size
            if ((vertexBufferContentSize + vertexBufferAdditionalContentSize > vertexBufferSize) ||
                    (indexBufferContentSize + indexBufferAdditionalContentSize > indexBufferSize))  {
                logError("Can not update buffer since size is too small!");
                bufferTooSmall = true;
                break;
            }

            additionalVertices.insert(additionalVertices.end(), mesh.vertices.begin(), mesh.vertices.end());
            additionalIndices.insert(additionalIndices.end(), mesh.indices.begin(), mesh.indices.end());
        }

        if (bufferTooSmall) break;
        additionalObjectsAdded++;
    }

    if (additionalObjectsAdded == 0) return true;

    if (!this->addObjectsToBeRendererCommon(additionalVertices, additionalIndices)) return false;

    this->objectsToBeRendered.insert(
        this->objectsToBeRendered.end(), additionalObjectsToBeRendered.begin(),
        additionalObjectsToBeRendered.begin() + additionalObjectsAdded
    );

    return true;
}

void DynamicObjectsColorMeshPipeline::draw(const VkCommandBuffer& commandBuffer, const uint16_t commandBufferIndex)
{
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
        for (const auto & m : o->getMeshes()) {
            const VkDeviceSize vertexCount = m.vertices.size();
            const VkDeviceSize indexCount = m.indices.size();

            if (o->shouldBeRendered(Camera::INSTANCE()->getFrustumPlanes())) {
                const DynamicColorMeshPushConstants & pushConstants = { o->getMatrix(), m.color };
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

void DynamicObjectsColorMeshPipeline::update() {}


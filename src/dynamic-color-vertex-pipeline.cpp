#include "includes/engine.h"

DynamicObjectsColorVertexPipeline::DynamicObjectsColorVertexPipeline(const std::string name, Renderer * renderer) : StaticObjectsColorVertexPipeline(name, renderer) { }

bool DynamicObjectsColorVertexPipeline::createPipeline()
{
    if (!this->createDescriptors()) {
        logError("Failed to create '" + this->name + "' Pipeline Descriptors");
        return false;
    }

    return this->createGraphicsPipelineCommon(true, true, true, this->config.topology);
}

bool DynamicObjectsColorVertexPipeline::initPipeline(const PipelineConfig & config)
{
    try {
        this->config = std::move(dynamic_cast<const DynamicObjectsColorVertexPipelineConfig &>(config));
    } catch (std::bad_cast ex) {
        logError("'" + this->name + "' Pipeline needs instance of DynamicColorVertexPipelineConfig!");
        return false;
    }

    this->pushConstantRange = VkPushConstantRange {};
    this->pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    this->pushConstantRange.offset = 0;
    this->pushConstantRange.size = sizeof(glm::mat4);

    for (const auto & s : this->config.shaders) {
        if (!this->addShader((Engine::getAppPath(SHADERS) / s.file).string(), s.shaderType)) {
            logError("Failed to add shader: " + s.file);
        }
    }

    if (this->getNumberOfValidShaders() < 2) {
        logError(" '" + this->name + "' Pipeline needs vertex and fragment shaders at a minimum!");
        return false;
    }

    if (!this->createBuffers()) {
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

void DynamicObjectsColorVertexPipeline::draw(const VkCommandBuffer& commandBuffer, const uint16_t commandBufferIndex)
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
        const VkDeviceSize vertexCount = o->getVertices().size();
        const VkDeviceSize indexCount = o->getIndices().size();

        if (o->shouldBeRendered(Camera::INSTANCE()->getFrustumPlanes())) {
            const glm::mat4 & m = o->getMatrix();
            vkCmdPushConstants(commandBuffer, this->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m) , &m);

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

void DynamicObjectsColorVertexPipeline::update() {}

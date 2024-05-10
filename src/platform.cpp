#include "includes/graphics.h"
#include "includes/engine.h"
#include "includes/geometry.h"

std::filesystem::path Engine::base  = "";

class TestPipeline : public GraphicsPipeline
{
private:
    // TODO: combine vertices and indexes
    // TODO: find good memory backing model that scales

    std::vector<ColorVertex> vertices;
    std::vector<ColorVertex> indexes;

    void createTestVertices() {
        for (int i=-100;i<100;i+=10) {
            for (int j=-100;j<100;j+=10) {
                for (int k=-100;k<100;k+=10) {
                    this->vertices.push_back({
                        glm::vec3(i, j, k),
                        glm::vec3(0.0, 0.0, 0.0),
                        glm::vec3(1.0, 1.0, 1.0)
                    });
                    this->vertices.push_back({
                        glm::vec3(i, j+2, k),
                        glm::vec3(0.0, 0.0, 0.0),
                        glm::vec3(1.0, 1.0, 1.0)
                    });
                    this->vertices.push_back({
                        glm::vec3(i-2, j, k),
                        glm::vec3(0.0, 0.0, 0.0),
                        glm::vec3(1.0, 1.0, 1.0)
                    });
                }
            }
        }
    }

    bool createBuffersForTestVertices() {
        if (this->vertices.empty()) return true;

        Buffer stagingBuffer;

        VkDeviceSize contentSize = this->vertices.size() * sizeof(ColorVertex);

        logInfo("Memory Usage: " + Helper::formatMemoryUsage(contentSize));

        this->vertexBuffer.destroy(this->renderer->getLogicalDevice());
        this->vertexBuffer.createDeviceLocalBuffer(
            stagingBuffer, 0, contentSize,
            this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(),
            this->renderer->getGraphicsCommandPool(), this->renderer->getGraphicsQueue()
        );
        this->vertexBuffer.updateContentSize(contentSize);

        if (!this->vertexBuffer.isInitialized()) {
            logError("Failed to create Test Vertex Buffer!");
            return false;
        }

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
        }

        if (this->indexes.empty()) return true;

        contentSize = this->indexes.size() * sizeof(uint32_t);
        this->indexBuffer.destroy(this->renderer->getLogicalDevice());
        this->indexBuffer.createDeviceLocalBuffer(
            stagingBuffer, 0, contentSize,
            this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(),
            this->renderer->getGraphicsCommandPool(), this->renderer->getGraphicsQueue()
        );
        this->indexBuffer.updateContentSize(contentSize);

        if (!this->indexBuffer.isInitialized()) {
            logError("Failed to create Test Index Buffer!");
            return false;
        }

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
        }

        return true;
    }

public:
    TestPipeline(Renderer * renderer) : GraphicsPipeline(renderer) { }

    bool createPipeline() {
        if (!this->createDescriptors()) {
            logError("Failed to create Models Pipeline Descriptors");
            return false;
        }

        return this->createGraphicsPipelineCommon();
    }

    bool initPipeline() {
        this->createTestVertices();

        if (!this->createBuffersForTestVertices()) {
            logError("Failed to create Test Buffers");
            return false;
        }

        if (!this->createDescriptorPool()) {
            logError("Failed to create Test Pipeline Descriptor Pool");
            return false;
        }

        return this->createPipeline();
    }

    void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex) {
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

    void update() {}

    bool createDescriptors() {
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

    bool createDescriptorPool() {
        if (this->renderer == nullptr || !this->renderer->isReady() || this->descriptorPool.isInitialized()) return false;

        const uint32_t count = this->renderer->getImageCount();

        this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
        this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);

        this->descriptorPool.createPool(this->renderer->getLogicalDevice(), count);

        return this->descriptorPool.isInitialized();
    }
};

int start(int argc, char* argv []) {
    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();

    // create mininal test pipeline for now
    // to check basic functionality after project imports
    auto pipe = std::make_unique<TestPipeline>(engine->getRenderer());
    pipe->addShader((Engine::getAppPath(SHADERS) / "test.vert.spv").string(), VK_SHADER_STAGE_VERTEX_BIT);
    pipe->addShader((Engine::getAppPath(SHADERS) / "test.frag.spv").string(), VK_SHADER_STAGE_FRAGMENT_BIT);

    if (!pipe->initPipeline()) {
        logError("Failed to init Test Pipeline");
    }

    engine->addPipeline(pipe.release());

    engine->loop();

    return 0;
}

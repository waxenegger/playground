#include "includes/engine.h"

SkyboxPipeline::SkyboxPipeline(const std::string name, Renderer * renderer) : GraphicsPipeline(name, renderer) { }

bool SkyboxPipeline::initPipeline(const PipelineConfig & config) {
    if (this->renderer == nullptr || !this->renderer->isReady()) return false;

    try {
        this->config = std::move(dynamic_cast<const SkyboxPipelineConfig &>(config));
    } catch (std::bad_cast ex) {
        logError("SkyboxPipeline needs instance of SkyboxPipelineConfig!");
        return false;
    }

    if (this->config.skyboxImages.size() != 6) {
        logError("Skybox config needs 6 image locations!");
        return false;
    }

    for (const auto & s : this->config.shaders) {
        if (!this->addShader((Engine::getAppPath(SHADERS) / s.file).string(), s.shaderType)) {
            logError("Failed to add shader: " + s.file);
        }
    }

    if (this->getNumberOfValidShaders() < 2) {
        logError("SkyboxPipeline needs vertex and fragment shaders at a minimum!");
        return false;
    }

    this->skyboxTextures.clear();
    for (auto & s : this->config.skyboxImages) {
        auto skyTexture = GlobalTextureStore::INSTANCE()->getTextureByName(s);

        if (skyTexture == nullptr) {
            std::unique_ptr<Texture> texture = std::make_unique<Texture>();
            texture->setPath(Engine::getAppPath(IMAGES) / s);
            texture->load();

            if (!texture->isValid()) {
                logError("Could not load Skybox Texture: " + texture->getPath());
                return false;
            }

            int textureIndex = GlobalTextureStore::INSTANCE()->addTexture(s , texture);
            if (textureIndex < 0) return false;

            skyTexture = GlobalTextureStore::INSTANCE()->getTextureByIndex(textureIndex);
        }

        this->skyboxTextures.push_back(skyTexture);
    }

    if (!this->createSkybox()) {
        logError("Failed to create Skybox Pipeline Texture Sampler");
        return false;
    }

    if (!this->createDescriptorPool()) {
        logError("Failed to create Skybox Pipeline Descriptor Pool");
        return false;
    }

    return this->createPipeline();
}

void SkyboxPipeline::update() {}

bool SkyboxPipeline::createPipeline() {
    if (!this->createDescriptors()) {
        logError("Failed to create Skybox Pipeline Descriptors");
        return false;
    }

    return this->createGraphicsPipelineCommon(false, false, false);
}

bool SkyboxPipeline::createDescriptorPool() {
    if (this->renderer == nullptr || !this->renderer->isReady() || this->descriptorPool.isInitialized()) return false;

    const uint32_t count = this->renderer->getImageCount();

    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count);

    this->descriptorPool.createPool(this->renderer->getLogicalDevice(), count);

    return this->descriptorPool.isInitialized();
}

bool SkyboxPipeline::createDescriptors() {
    if (this->renderer == nullptr || !this->renderer->isReady()) return false;

    this->descriptors.destroy(this->renderer->getLogicalDevice());
    this->descriptorPool.resetPool(this->renderer->getLogicalDevice());

    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
    this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

    this->descriptors.create(this->renderer->getLogicalDevice(), this->descriptorPool.getPool(), this->renderer->getImageCount());

    if (!this->descriptors.isInitialized()) return false;

    const uint32_t descSize = this->descriptors.getDescriptorSets().size();
    for (size_t i = 0; i < descSize; i++) {
        const VkDescriptorBufferInfo & uniformBufferInfo = this->renderer->getUniformBuffer(i).getDescriptorInfo();
        this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), 0, i, uniformBufferInfo);

        const VkDescriptorBufferInfo & vertexDeviceLocalBufferInfo = this->vertexBuffer.getDescriptorInfo();
        this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), 1, i, vertexDeviceLocalBufferInfo);

        const std::vector<VkDescriptorImageInfo> & cubeImageInfos = { this->cubeImage.getDescriptorInfo() };
        this->descriptors.updateWriteDescriptorWithImageInfo(this->renderer->getLogicalDevice(), 2, i, cubeImageInfos);
    }

    return true;
}

bool SkyboxPipeline::createSkybox() {
    if (this->renderer == nullptr || !this->renderer->isReady()) return false;

    this->vertexBuffer.destroy(this->renderer->getLogicalDevice());

    if (this->skyboxTextures.size() != 6) return false;

    VkDeviceSize bufferSize = this->SKYBOX_VERTICES.size() * sizeof(glm::vec3);

    Buffer stagingBuffer;
    stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), bufferSize);
    if (!stagingBuffer.isInitialized()) {
        logError("Failed to create Skybox Vertex Staging Buffer!");
        return false;
    }

    memcpy(stagingBuffer.getBufferData(), this->SKYBOX_VERTICES.data(), bufferSize);
    stagingBuffer.updateContentSize(bufferSize);

    this->vertexBuffer.createDeviceLocalBuffer(
        stagingBuffer, 0, stagingBuffer.getContentSize(),
        this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(),
        this->renderer->getGraphicsCommandPool(), this->renderer->getGraphicsQueue()
    );
    stagingBuffer.destroy(this->renderer->getLogicalDevice());

    if (!this->vertexBuffer.isInitialized()) {
        logError("Failed to create Skybox Vertex Buffer!");
        return false;
    }

    bufferSize = this->SKYBOX_INDEXES.size() * sizeof(uint32_t);
    stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice() , bufferSize);
    if (!stagingBuffer.isInitialized()) {
        logError("Failed to create Skybox Index Staging Buffer!");
        return false;
    }

    memcpy(stagingBuffer.getBufferData(), SKYBOX_INDEXES.data(), bufferSize);
    stagingBuffer.updateContentSize(bufferSize);

    this->indexBuffer.createDeviceLocalBuffer(
        stagingBuffer, 0, stagingBuffer.getContentSize(),
        this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(),
        this->renderer->getGraphicsCommandPool(), this->renderer->getGraphicsQueue(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );
    stagingBuffer.destroy(this->renderer->getLogicalDevice());

    if (!this->indexBuffer.isInitialized()) {
        logError("Failed to create Skybox Index Buffer!");
        return false;
    }

    this->cubeImage.destroy(this->renderer->getLogicalDevice());

    VkDeviceSize skyboxCubeSize = this->skyboxTextures.size() * this->skyboxTextures[0]->getSize();
    stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), skyboxCubeSize);
    if (!stagingBuffer.isInitialized()) {
        logError("Failed to create Skybox Textures Staging Buffer!");
        return false;
    }

    VkDeviceSize offset = 0;
    for (auto & tex : this->skyboxTextures) {
        memcpy(static_cast<char *>(stagingBuffer.getBufferData()) + offset, tex->getPixels(), tex->getSize());
        offset += tex->getSize();
    }

    ImageConfig conf;
    conf.isDepthImage = false;
    conf.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    conf.format = this->skyboxTextures[0]->getImageFormat();
    conf.width =  this->skyboxTextures[0]->getWidth();
    conf.height =  this->skyboxTextures[0]->getHeight();
    conf.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    conf.arrayLayers = this->skyboxTextures.size();

    this->cubeImage.createImage(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), conf);

    if (!cubeImage.isInitialized()) {
        stagingBuffer.destroy(this->renderer->getLogicalDevice());
        logError("Failed to Create Skybox Images");
        return false;
    }

    const CommandPool & pool = this->renderer->getGraphicsCommandPool();
    const VkCommandBuffer & commandBuffer = pool.beginPrimaryCommandBuffer(this->renderer->getLogicalDevice());

    this->cubeImage.transitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, this->skyboxTextures.size());
    this->cubeImage.copyBufferToImage(
        commandBuffer, stagingBuffer.getBuffer(), this->skyboxTextures[0]->getWidth(),
        this->skyboxTextures[0]->getHeight(), this->skyboxTextures.size()
    );
    this->cubeImage.transitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, this->skyboxTextures.size());

    pool.endCommandBuffer(commandBuffer);
    pool.submitCommandBuffer(this->renderer->getLogicalDevice(), this->renderer->getGraphicsQueue(), commandBuffer);

    stagingBuffer.destroy(this->renderer->getLogicalDevice());

    return true;
}

void SkyboxPipeline::draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex) {
    if (this->hasPipeline() && this->isEnabled() && this->vertexBuffer.isInitialized() && this->indexBuffer.isInitialized()) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->layout, 0, 1, &this->descriptors.getDescriptorSets()[commandBufferIndex], 0, nullptr);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

        vkCmdBindIndexBuffer(commandBuffer, this->indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

        this->correctViewPortCoordinates(commandBuffer);

        vkCmdDrawIndexed(commandBuffer, this->SKYBOX_INDEXES.size(), 1, 0, 0, 0);
    }
}

SkyboxPipeline::~SkyboxPipeline() {
    if (this->renderer == nullptr || !this->renderer->isReady()) return;

     this->cubeImage.destroy(this->renderer->getLogicalDevice());
}


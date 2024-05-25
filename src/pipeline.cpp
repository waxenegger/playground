#include "includes/engine.h"

Pipeline::Pipeline(const std::string name, Renderer * renderer) : name(name), renderer(renderer) {
    if (renderer == nullptr) {
        logError("Pipeline construction requires a renderer instance!");
    }
}

std::string Pipeline::getName() const {
    return this->name;
}

void Pipeline::setName(const std::string name) {
    this->name = name;
}

Pipeline::~Pipeline() {
    for (auto & shader : this->shaders) {
        if (shader.second != nullptr) {
            delete shader.second;
            shader.second = nullptr;
        }
    }
    this->shaders.clear();

    this->descriptors.destroy(this->renderer->getLogicalDevice());
    this->descriptorPool.destroy(this->renderer->getLogicalDevice());

    this->destroyPipeline();
}

uint8_t Pipeline::getNumberOfValidShaders() const
{
    uint8_t validShaders = 0;

    for (auto & shader : this->shaders) {
        if (shader.second->isValid()) validShaders++;
    }

    return validShaders;
}

void Pipeline::setEnabled(const bool flag) {
    this->enabled = flag;
}

bool Pipeline::isEnabled() {
    return this->enabled;
}

std::vector<VkPipelineShaderStageCreateInfo> Pipeline::getShaderStageCreateInfos() {
    std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfos;

    for (auto & shader : this->shaders) {
        if (shader.second != nullptr && shader.second->isValid()) {
            VkPipelineShaderStageCreateInfo shaderStageInfo{};

            shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageInfo.stage = shader.second->getShaderType();
            shaderStageInfo.module = shader.second->getShaderModule();
            shaderStageInfo.pName = "main";

            shaderCreateInfos.push_back(shaderStageInfo);
        }
    }

    return shaderCreateInfos;
}

bool Pipeline::hasPipeline() const {
    return this->pipeline != nullptr;
}

void Pipeline::destroyPipeline() {
    if (this->renderer == nullptr) return;

    if (this->pipeline != nullptr) {
        vkDestroyPipeline(this->renderer->getLogicalDevice(), this->pipeline, nullptr);
        this->pipeline = nullptr;
    }

    if (this->layout != nullptr) {
        vkDestroyPipelineLayout(this->renderer->getLogicalDevice(), this->layout, nullptr);
        this->layout = nullptr;
    }
}

bool Pipeline::addShader(const std::string & filename, const VkShaderStageFlagBits & shaderType) {
    if (this->renderer == nullptr) return false;

    const std::map<std::string, const Shader *>::iterator existingShader = this->shaders.find(filename);
    if (existingShader != this->shaders.end()) {
        logInfo("Shader " + filename + " already exists!");
        return false;
    }

    std::unique_ptr<Shader> newShader = std::make_unique<Shader>(this->renderer->getLogicalDevice(), filename, shaderType);
    if (newShader->isValid()) {
        this->shaders[filename] = newShader.release();
        return true;
    }

    return false;
}


GraphicsPipeline::GraphicsPipeline(const std::string name, Renderer * renderer) : Pipeline(name, renderer) {}

bool GraphicsPipeline::createGraphicsPipelineCommon(const bool doColorBlend, const bool hasDepth, const bool cullBack, const VkPrimitiveTopology topology)
{
    if (this->renderer == nullptr) {
        logError("Graphics Pipeline needs renderer instance!");
        return false;
    }

    this->destroyPipeline();

    VkGraphicsPipelineCreateInfo pipelineCreateInfo {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = nullptr;
    pipelineCreateInfo.flags = 0;

    // topology and drawing primitives
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;

    // vertex info and shaders
    const std::vector<VkPipelineShaderStageCreateInfo> & shaderStageCreateInfos = this->getShaderStageCreateInfos();
    VkPipelineVertexInputStateCreateInfo vertexInputState {};
    vertexInputState.sType =  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStageCreateInfos.size());
    pipelineCreateInfo.pStages = shaderStageCreateInfos.data();

    // viewport
    const VkExtent2D & swapChainExtent = this->renderer->getSwapChainExtent();

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = swapChainExtent.width;
    viewport.height = swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkDynamicState dynamicState = VK_DYNAMIC_STATE_VIEWPORT;
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pNext = nullptr;
    dynamicStateCreateInfo.dynamicStateCount = 1;
    dynamicStateCreateInfo.pDynamicStates = &dynamicState;
    pipelineCreateInfo.pDynamicState = (&dynamicStateCreateInfo);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    pipelineCreateInfo.pViewportState = &viewportState;

    // rasterizer (incl. wireframe option)
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = this->renderer->doesShowWireFrame()  ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = cullBack ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_FRONT_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    pipelineCreateInfo.pRasterizationState = &rasterizer;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineCreateInfo.pMultisampleState = &multisampling;

    // color attachment and blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    if (doColorBlend) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.blendEnable = VK_TRUE;
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    pipelineCreateInfo.pColorBlendState = &colorBlending;

    // depth settings
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = hasDepth ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = hasDepth ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pSetLayouts = this->descriptors.getDescriptorSetLayout() == nullptr ? VK_NULL_HANDLE : &this->descriptors.getDescriptorSetLayout();
    pipelineLayoutInfo.setLayoutCount = this->descriptors.getDescriptorSetLayout() == nullptr ? 0 : 1;

    if (this->pushConstantRange.size > 0) {
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &this->pushConstantRange;
    }

    VkResult ret = vkCreatePipelineLayout(this->renderer->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &this->layout);
    if (ret != VK_SUCCESS) {
        logError("Failed to Create Graphics Pipeline Layout!");
        return false;
    }

    pipelineCreateInfo.layout = this->layout;
    pipelineCreateInfo.renderPass = this->renderer->getRenderPass();
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    ret = vkCreateGraphicsPipelines(this->renderer->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &this->pipeline);
    if (ret != VK_SUCCESS) {
        logError("Failed to Create Pipeline!");
        return false;
    }

    return true;
}

bool GraphicsPipeline::canRender() const
{
    return true;
}

bool GraphicsPipeline::isReady() const {
    uint8_t validShaders = this->getNumberOfValidShaders();
    return this->hasPipeline() && validShaders >= 2;
}

void GraphicsPipeline::correctViewPortCoordinates(const VkCommandBuffer & commandBuffer) {
    if (!this->isEnabled() || !this->isReady()) return;

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = (float)this->renderer->getSwapChainExtent().height;
    viewport.width = (float)this->renderer->getSwapChainExtent().width;
    viewport.height = -(float)this->renderer->getSwapChainExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}

GraphicsPipeline::~GraphicsPipeline() {
    this->destroyPipeline();

    if (this->textureSampler != nullptr) {
        vkDestroySampler(this->renderer->getLogicalDevice(), this->textureSampler, nullptr);
        this->textureSampler = nullptr;
    }

    if (this->usesDeviceLocalVertexBuffer) {
        this->renderer->trackDeviceLocalMemory(this->vertexBuffer.getSize(), true);
    }
    this->vertexBuffer.destroy(this->renderer->getLogicalDevice());

    if (this->usesDeviceLocalIndexBuffer) {
        this->renderer->trackDeviceLocalMemory(this->indexBuffer.getSize(), true);
    }
    this->indexBuffer.destroy(this->renderer->getLogicalDevice());

    this->ssboMeshBuffer.destroy(this->renderer->getLogicalDevice());
    this->ssboInstanceBuffer.destroy(this->renderer->getLogicalDevice());
    this->animationMatrixBuffer.destroy(this->renderer->getLogicalDevice());
}

PipelineFactory::PipelineFactory(Renderer* renderer) : renderer(renderer) {}

Pipeline * PipelineFactory::create(const std::string & name, const PipelineConfig & pipelineConfig)
{

    try {
        switch(pipelineConfig.getType()) {
            case GenericGraphics:
            {
                return this->create(name, dynamic_cast<const GenericGraphicsPipelineConfig &>(pipelineConfig));
            }
            case SkyBox:
            {
                return this->create(name, dynamic_cast<const SkyboxPipelineConfig &>(pipelineConfig));
            }
            case StaticObjectsColor:
            {
                return this->create(name, dynamic_cast<const StaticObjectsColorVertexPipelineConfig &>(pipelineConfig));
            }
            case DynamicObjectsColor:
            {
                return this->create(name, dynamic_cast<const DynamicObjectsColorVertexPipelineConfig &>(pipelineConfig));
            }
            case ImGUI:
            {
                return this->create(name, dynamic_cast<const ImGUIPipelineConfig &>(pipelineConfig));
            }
            case Unknown:
            default:
                logError("No generic Pipeline type can be created yet");
                break;
        };
    } catch (std::bad_cast) {
        logError("Passed a wrong pipeline config as in type set does not fit struct layout");
    }

    return nullptr;
}

Pipeline * PipelineFactory::create(const std::string & name, const StaticObjectsColorVertexPipelineConfig & staticObjectsColorVertexPipelineConfig)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<StaticObjectsColorVertexPipeline>(name, this->renderer);

    if (!pipe->initPipeline(staticObjectsColorVertexPipelineConfig)) {
        logError("Failed to init Pipeline: " + name);
        return nullptr;
    }

    return pipe.release();
}

Pipeline * PipelineFactory::create(const std::string & name, const DynamicObjectsColorVertexPipelineConfig & dynamicObjectsColorVertexPipelineConfig)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<DynamicObjectsColorVertexPipeline>(name, this->renderer);

    if (!pipe->initPipeline(dynamicObjectsColorVertexPipelineConfig)) {
        logError("Failed to init Pipeline: " + name);
        return nullptr;
    }

    return pipe.release();
}

Pipeline * PipelineFactory::create(const std::string & name, const SkyboxPipelineConfig & skyboxPipelineConfig)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<SkyboxPipeline>(name, this->renderer);

    if (!pipe->initPipeline(skyboxPipelineConfig)) {
        logError("Failed to init Pipeline: " + name);
        return nullptr;
    }

    return pipe.release();
}

Pipeline * PipelineFactory::create(const std::string & name, const GenericGraphicsPipelineConfig & genericGraphicsPipelineConfig)
{
    logError("Falls back onto StaticObjectsColorVertexPipeline for now");

    StaticObjectsColorVertexPipelineConfig conf;
    conf.topology = genericGraphicsPipelineConfig.topology;
    conf.enableColorBlend =genericGraphicsPipelineConfig.enableColorBlend;
    conf.enableDepth = genericGraphicsPipelineConfig.enableDepth;

    return this->create(name, conf);
}

Pipeline * PipelineFactory::create(const std::string & name, const ImGUIPipelineConfig & imGuiPipelineConfig)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<ImGuiPipeline>(name, this->renderer);

    if (!pipe->initPipeline(imGuiPipelineConfig)) {
        logError("Failed to init Pipeline: " + name);
        return nullptr;
    }

    return pipe.release();
}

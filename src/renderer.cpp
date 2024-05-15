#include "includes/pipelines.h"

Renderer::Renderer(const GraphicsContext * graphicsContext, const VkPhysicalDevice & physicalDevice, const int & graphicsQueueIndex) :
    graphicsContext(graphicsContext), physicalDevice(physicalDevice), graphicsQueueIndex(graphicsQueueIndex) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    const float priorities[] = {1.0f,1.0f};

    VkDeviceQueueCreateInfo queueCreateInfo {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.flags = 0;
    queueCreateInfo.pNext = nullptr;
    queueCreateInfo.queueFamilyIndex = this->graphicsQueueIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &priorities[0];

    queueCreateInfos.push_back(queueCreateInfo);

    const std::vector<const char * > extensionsToEnable = {
        "VK_KHR_swapchain",
        //"VK_KHR_shader_draw_parameters",
        //"VK_KHR_shader_non_semantic_info"
    };

    VkPhysicalDeviceFeatures2 deviceFeatures { };
    deviceFeatures.sType =  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.features.samplerAnisotropy = VK_TRUE;
    deviceFeatures.features.multiDrawIndirect = VK_FALSE;
    deviceFeatures.features.fillModeNonSolid = VK_TRUE;
    deviceFeatures.features.geometryShader = VK_TRUE;

    VkPhysicalDeviceVulkan12Features vulkan12Features { };
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.drawIndirectCount = true;

    deviceFeatures.pNext = &vulkan12Features;

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &deviceFeatures;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    //createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.ppEnabledExtensionNames = extensionsToEnable.data();
    createInfo.enabledExtensionCount = extensionsToEnable.size();

    const VkResult ret = vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->logicalDevice);
    if (ret != VK_SUCCESS) {
        logError("Failed to create Logical Device!");
        return;
    }

    this->queryPhysicalDeviceProperties();

    vkGetDeviceQueue(this->logicalDevice, this->graphicsQueueIndex , 0, &this->graphicsQueue);

    vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &this->memoryProperties);
}

void Renderer::queryPhysicalDeviceProperties(const bool log) {
    if (this->physicalDevice == nullptr) return;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(this->physicalDevice, &properties);

    this->deviceProperties["maxUniformBufferRange"] = properties.limits.maxUniformBufferRange;
    this->deviceProperties["maxStorageBufferRange"] = properties.limits.maxStorageBufferRange;
    this->deviceProperties["maxPushConstantsSize"] = properties.limits.maxPushConstantsSize;
    this->deviceProperties["maxMemoryAllocationCount"] = properties.limits.maxMemoryAllocationCount;
    this->deviceProperties["maxComputeSharedMemorySize"] = properties.limits.maxComputeSharedMemorySize;

    if (!log) return;

    logInfo("Some Physical Device Properties...");
    for (auto & entry : this->deviceProperties) {
        logInfo(entry.first + ": " + Helper::formatMemoryUsage(entry.second));
    }
}

uint32_t Renderer::getMaxMemoryLimit(const std::string type) {
    auto res = this->deviceProperties.find(type);
    if (res == this->deviceProperties.end()) return 0;

    return res->second;
}

bool Renderer::isReady() const {
    return this->graphicsContext != nullptr && this->physicalDevice != nullptr && this->logicalDevice  != nullptr;
}

bool Renderer::hasAtLeastOneActivePipeline() const {
    bool isReady = false;

    for (Pipeline * pipeline : this->pipelines) {
        if (pipeline != nullptr && pipeline->isReady()) {
            isReady = true;
            break;
        }
    }

    return isReady;
}

bool Renderer::createUniformBuffers() {
    if (!this->isReady()) return false;

    VkDeviceSize bufferSize = sizeof(struct GraphicsUniforms);

    bool initialized = true;

    this->uniformBuffer = std::vector<Buffer>(this->imageCount);
    for (auto & b : this->uniformBuffer) {
        b.createSharedUniformBuffer(this->getPhysicalDevice(), this->getLogicalDevice(), bufferSize);
        if (!b.isInitialized()) {
            initialized = false;
        }
    }

    return initialized;
}

void Renderer::updateUniformBuffers(int index, uint32_t componentsDrawCount) {
    GraphicsUniforms graphUniforms {};
    const glm::vec4 pos(Camera::INSTANCE()->getPosition(), 1.0f);
    graphUniforms.camera = pos;
    graphUniforms.viewProjMatrix = Camera::INSTANCE()->getProjectionMatrix() * Camera::INSTANCE()->getViewMatrix();

    memcpy(this->uniformBuffer[index].getBufferData(), &graphUniforms, sizeof(graphUniforms));
}

const Buffer & Renderer::getUniformBuffer(int index) const {
    return this->uniformBuffer[index];
}

Pipeline * Renderer::getPipeline(const std::string name) {
    if (this->pipelines.empty()) return nullptr;

    for (auto & p : this->pipelines) {
        if (p->getName() == name) return p;
    }

    return nullptr;
}

bool Renderer::canRender() const
{
    bool graphicCanRender =
        this->isReady() && this->hasAtLeastOneActivePipeline() && this->swapChain != nullptr && this->swapChainImages.size() == this->imageCount &&
        this->imageAvailableSemaphores.size() == this->imageCount && this->renderFinishedSemaphores.size() == this->imageCount && this->inFlightFences.size() == this->imageCount &&
        this->swapChainFramebuffers.size() == this->swapChainImages.size() && this->depthImages.size() == this->swapChainImages.size() && this->depthImages.size() == this->imageCount &&
        this->graphicsCommandPool.isInitialized();
    if (!graphicCanRender) return false;

    return true;
}


bool Renderer::addPipeline(std::unique_ptr<Pipeline> pipeline) {
    if (!this->isReady()) {
        logError("Render has not been properly initialized!");
        return false;
    }

    auto p = this->getPipeline(pipeline->getName());
    if (p != nullptr) {
        logError("There exists already a pipeline by the same name!");
        return false;
    }

    const bool wasPaused = this->isPaused();
    if (!wasPaused) {
        this->pause();
    }

    this->pipelines.push_back(pipeline.release());

    if (!wasPaused) {
        this->forceRenderUpdate();
        this->resume();
    }

    return this->pipelines.size()-1;
}

void Renderer::enablePipeline(const std::string name, const bool flag) {
    if (!this->isReady()) {
        logError("Render has not been properly initialized!");
        return;
    }

    if (this->pipelines.empty()) return;

    auto p = this->getPipeline(name);
    if (p == nullptr) return;

    p->setEnabled(flag);
}

void Renderer::removePipeline(const std::string name) {
    if (!this->isReady() || this->pipelines.empty()) {
        return;
    }

    const bool wasPaused = this->isPaused();
    if (!wasPaused) {
        this->pause();
    }

    int idx = -1, i=0;
    for (auto & p : this->pipelines) {
        if (p->getName() == name) {
            idx = i;
            break;
        }
        i++;
    }

    if (idx != -1) this->pipelines.erase(this->pipelines.begin() + idx);

    if (!wasPaused) {
        this->forceRenderUpdate();
        this->resume();
    }
}

bool Renderer::createRenderPass() {
    if (!this->isReady()) {
        logError("Renderer has not been initialized!");
        return false;
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = SWAP_CHAIN_IMAGE_FORMAT.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkFormat depthFormat;
    if (!GraphicsContext::findDepthFormat(this->physicalDevice, depthFormat)) {
        logError("Failed to Find Depth Format!");
    }

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::vector<VkSubpassDependency> dependencies {
        {
            VK_SUBPASS_EXTERNAL,
            0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,0
        }
    };

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = dependencies.size();
    renderPassInfo.pDependencies = dependencies.data();

    VkResult ret = vkCreateRenderPass(this->logicalDevice, &renderPassInfo, nullptr, &this->renderPass);
    if (ret != VK_SUCCESS) {
       logError("Failed to Create Render Pass!");
       return false;
    }

    return true;
}

bool Renderer::createSwapChain() {
    if (!this->isReady()) {
        logError("Renderer has not been initialized!");
        return false;
    }

    const std::vector<VkPresentModeKHR> presentModes = this->graphicsContext->queryDeviceSwapModes(this->physicalDevice);
    if (presentModes.empty()) {
        logError("Swap Modes Require Surface!");
        return false;
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (!this->graphicsContext->getSurfaceCapabilities(this->physicalDevice, surfaceCapabilities)) return false;

    this->swapChainExtent = this->graphicsContext->getSwapChainExtent(surfaceCapabilities);

    this->maximized =
    #if (USE_GLFW == 0)
        (SDL_GetWindowFlags(this->graphicsContext->getSdlWindow()) & SDL_WINDOW_MAXIMIZED);
    #else
        glfwGetWindowAttrib(this->graphicsContext->getGlfwWindow(), GLFW_MAXIMIZED);
    #endif

    this->fullScreen =
    #if (USE_GLFW == 0)
        (SDL_GetWindowFlags(this->graphicsContext->getSdlWindow()) & SDL_WINDOW_FULLSCREEN);
    #else
        glfwGetWindowMonitor(this->graphicsContext->getGlfwWindow()) != nullptr;
    #endif

    if (surfaceCapabilities.maxImageCount > 0 && this->imageCount > surfaceCapabilities.maxImageCount) {
        this->imageCount = surfaceCapabilities.maxImageCount;
    }

    VkPresentModeKHR presentSwapMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto & swapMode : presentModes) {
        if (swapMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentSwapMode = swapMode;
            break;
        }
    }

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.surface = this->graphicsContext->getVulkanSurface();
    createInfo.minImageCount = this->imageCount;
    createInfo.imageFormat = SWAP_CHAIN_IMAGE_FORMAT.format;
    createInfo.imageColorSpace = SWAP_CHAIN_IMAGE_FORMAT.colorSpace;
    createInfo.imageExtent = this->swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.presentMode = presentSwapMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;

    VkResult ret = vkCreateSwapchainKHR(this->logicalDevice, &createInfo, nullptr, &this->swapChain);
    if (ret != VK_SUCCESS) {
        logError("Failed to Create Swap Chain!");
        return false;
    }

    ret = vkGetSwapchainImagesKHR(this->logicalDevice, this->swapChain, &this->imageCount, nullptr);
    if (ret != VK_SUCCESS) {
        logError("Failed to Get Swap Chain Image Count!");
        return false;
    }

    logInfo("Buffering: " + std::to_string(this->imageCount));
    this->swapChainImages = std::vector<Image>(this->imageCount);

    std::vector<VkImage> swapchainImgs(this->imageCount);
    ret = vkGetSwapchainImagesKHR(this->logicalDevice, this->swapChain, &this->imageCount, swapchainImgs.data());
    if (ret != VK_SUCCESS) {
        logError("Failed to Get Swap Chain Images!");
        return false;
    }

    for (uint32_t j=0;j<swapchainImgs.size();j++) {
        this->swapChainImages[j].createFromSwapchainImages(this->getLogicalDevice(), swapchainImgs[j]);
        if (!this->swapChainImages[j].isInitialized()) {
            logError("Failed to Create Swap Chain Images!");
            return false;
        }
    }

    return true;
}

bool Renderer::createSyncObjects() {
    if (!this->isReady()) {
        logError("Renderer has not been initialized!");
        return false;
    }

    this->imageAvailableSemaphores.resize(this->imageCount);
    this->renderFinishedSemaphores.resize(this->imageCount);
    this->inFlightFences.resize(this->imageCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < this->imageCount; i++) {

        if (vkCreateSemaphore(this->logicalDevice, &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(this->logicalDevice, &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(this->logicalDevice, &fenceInfo, nullptr, &this->inFlightFences[i]) != VK_SUCCESS) {
            logError("Failed to Create Synchronization Objects For Frame!");
            return false;
        }
    }

    return true;
}

bool Renderer::createCommandPools() {
    if (!this->isReady()) {
        logError("Renderer has not been initialized!");
        return false;
    }

    this->graphicsCommandPool.create(this->getLogicalDevice(), this->getGraphicsQueueIndex());

    return this->graphicsCommandPool.isInitialized();
}

const CommandPool & Renderer::getGraphicsCommandPool() const {
    return this->graphicsCommandPool;
}

VkQueue Renderer::getGraphicsQueue() const {
    return this->graphicsQueue;
}


bool Renderer::createFramebuffers() {
    if (!this->isReady()) {
        logError("Renderer has not been initialized!");
        return false;
    }

    this->swapChainFramebuffers.resize(this->swapChainImages.size());

     for (size_t i = 0; i < this->swapChainImages.size(); i++) {
         std::array<VkImageView, 2> attachments = {
             this->swapChainImages[i].getImageView(), this->depthImages[i].getImageView()
         };

         VkFramebufferCreateInfo framebufferInfo{};
         framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
         framebufferInfo.renderPass = renderPass;
         framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
         framebufferInfo.pAttachments = attachments.data();
         framebufferInfo.width = swapChainExtent.width;
         framebufferInfo.height = swapChainExtent.height;
         framebufferInfo.layers = 1;

         VkResult ret = vkCreateFramebuffer(this->logicalDevice, &framebufferInfo, nullptr, &this->swapChainFramebuffers[i]);
         if (ret != VK_SUCCESS) {
            logError("Failed to Create Frame Buffers!");
            return false;
         }
     }

     return true;
}

bool Renderer::createDepthResources() {
    this->depthImages = std::vector<Image>(this->swapChainImages.size());

    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    if (!GraphicsContext::findDepthFormat(this->physicalDevice, depthFormat)) {
        logError("Faild to create Depth Format!");
        return false;
    };

    for (uint16_t i=0;i<this->depthImages.size();i++) {
        this->depthImages[i].createImage(
            this->getPhysicalDevice(), this->getLogicalDevice(), depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            swapChainExtent.width, swapChainExtent.height, true);

        if (!this->depthImages[i].isInitialized()) {
            logError("Failed to create Depth Image!");
            return false;
        }
    }

    return true;
}

void Renderer::initRenderer() {
    if (!this->recreateRenderer()) return;
    if (!this->createCommandPools()) return;
    if (!this->createSyncObjects()) return;
    if (!this->createUniformBuffers()) return;
}

void Renderer::destroyRendererObjects() {
    this->destroySwapChainObjects();

    if (this->logicalDevice == nullptr) return;


    for (auto & b : this->uniformBuffer) {
        b.destroy(this->logicalDevice);
    }

    for (Pipeline * pipeline : this->pipelines) {
        if (pipeline != nullptr) {
            delete pipeline;
            pipeline = nullptr;
        }
    }
    pipelines.clear();

    for (size_t i = 0; i < this->imageCount; i++) {
        if (i < this->renderFinishedSemaphores.size()) {
            if (this->renderFinishedSemaphores[i] != nullptr) {
                vkDestroySemaphore(this->logicalDevice, this->renderFinishedSemaphores[i], nullptr);
            }
        }

        if (i < this->imageAvailableSemaphores.size()) {
            if (this->imageAvailableSemaphores[i] != nullptr) {
                vkDestroySemaphore(this->logicalDevice, this->imageAvailableSemaphores[i], nullptr);
            }
        }

        if (i < this->inFlightFences.size()) {
            if (this->inFlightFences[i] != nullptr) {
                vkDestroyFence(this->logicalDevice, this->inFlightFences[i], nullptr);
            }
        }
    }

    this->renderFinishedSemaphores.clear();
    this->imageAvailableSemaphores.clear();
    this->inFlightFences.clear();

    this->graphicsCommandPool.destroy(this->logicalDevice);
}

void Renderer::destroySwapChainObjects() {
    if (this->logicalDevice == nullptr) return;

    vkDeviceWaitIdle(this->logicalDevice);

    for (uint16_t j=0;j<this->depthImages.size();j++) {
        this->depthImages[j].destroy(this->logicalDevice);
    }

    this->depthImages.clear();

    for (auto & framebuffer : this->swapChainFramebuffers) {
        if (framebuffer != nullptr) {
            vkDestroyFramebuffer(this->logicalDevice, framebuffer, nullptr);
            framebuffer = nullptr;
        }
    }
    this->swapChainFramebuffers.clear();

    this->graphicsCommandPool.reset(this->logicalDevice);

    for (Pipeline * pipeline : this->pipelines) {
        if (pipeline != nullptr) {
            pipeline->destroyPipeline();
        }
    }

    if (this->renderPass != nullptr) {
        vkDestroyRenderPass(this->logicalDevice, this->renderPass, nullptr);
        this->renderPass = nullptr;
    }

    for (uint16_t j=0;j<this->swapChainImages.size();j++) {
        this->swapChainImages[j].destroy(this->logicalDevice, true);
    }
    this->swapChainImages.clear();

    if (this->swapChain != nullptr) {
        vkDestroySwapchainKHR(this->logicalDevice, this->swapChain, nullptr);
        this->swapChain = nullptr;
    }

}

VkDevice Renderer::getLogicalDevice() const {
    return this->logicalDevice;
}

VkPhysicalDevice Renderer::getPhysicalDevice() const {
    return this->physicalDevice;
}

void Renderer::destroyCommandBuffer(VkCommandBuffer commandBuffer, const bool resetOnly) {
    if (!this->isReady()) return;

    if (resetOnly) {
        this->graphicsCommandPool.resetCommandBuffer(commandBuffer);
    } else {
        this->graphicsCommandPool.freeCommandBuffer(this->logicalDevice, commandBuffer);
    }
}

VkCommandBuffer Renderer::createCommandBuffer(const uint16_t commandBufferIndex, const uint16_t imageIndex, const bool useSecondaryCommandBuffers) {
    if (this->requiresRenderUpdate) return nullptr;

    const VkCommandBuffer & commandBuffer = this->graphicsCommandPool.beginPrimaryCommandBuffer(this->logicalDevice);
    if (commandBuffer == nullptr) return nullptr;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->renderPass;
    renderPassInfo.framebuffer = this->swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = this->swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = this->clearValue;
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, useSecondaryCommandBuffers ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);

    for (Pipeline * pipeline : this->pipelines) {
        if (!this->requiresRenderUpdate && pipeline->isEnabled() && isReady()) {
            if (pipeline->canRender()) {
                static_cast<GraphicsPipeline *>(pipeline)->update();
                static_cast<GraphicsPipeline *>(pipeline)->draw(commandBuffer, commandBufferIndex);
            }
        }
    }

    vkCmdEndRenderPass(commandBuffer);

    this->graphicsCommandPool.endCommandBuffer(commandBuffer);

    return commandBuffer;
}

bool Renderer::createCommandBuffers() {
    this->commandBuffers.resize(this->swapChainFramebuffers.size());

    this->lastFrameRateUpdate = std::chrono::high_resolution_clock::now();

    return true;
}

void Renderer::render() {
    if (this->requiresRenderUpdate) {
        this->pause();
        this->recreateRenderer();
        this->resume();
    }

    if (this->paused) return;

    this->updateUniformBuffers(this->currentFrame);

    this->renderFrame();
}

void Renderer::renderFrame() {
    VkResult ret = vkWaitForFences(this->logicalDevice, 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
    if (ret != VK_SUCCESS) {
        this->requiresRenderUpdate = true;
        return;
    }

    ret = vkResetFences(this->logicalDevice, 1, &this->inFlightFences[this->currentFrame]);
    if (ret != VK_SUCCESS) {
        logError("Failed to Reset Fence!");
    }

    uint32_t imageIndex;
    ret = vkAcquireNextImageKHR(this->logicalDevice, this->swapChain, UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (ret != VK_SUCCESS) {
        if (ret != VK_ERROR_OUT_OF_DATE_KHR) {
            logError("Failed to Acquire Next Image");
        }
        this->requiresRenderUpdate = true;
        return;
    }

    if (this->commandBuffers[this->currentFrame] != nullptr) {
        this->graphicsCommandPool.freeCommandBuffer(this->logicalDevice, this->commandBuffers[this->currentFrame]);
    }

    this->commandBuffers[this->currentFrame] = this->createCommandBuffer(this->currentFrame, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    std::vector<VkSemaphore> waitSemaphores = {
        this->imageAvailableSemaphores[this->currentFrame],
    };

    std::vector<VkPipelineStageFlags> waitStages = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();

    submitInfo.commandBufferCount = this->commandBuffers.empty() ? 0 : 1;
    submitInfo.pCommandBuffers = &this->commandBuffers[this->currentFrame];

    VkSemaphore signalSemaphores[] = {this->renderFinishedSemaphores[this->currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    ret = vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, this->inFlightFences[this->currentFrame]);
    if (ret != VK_SUCCESS) {
        logError("Failed to Submit Draw Command Buffer!");
        this->requiresRenderUpdate = true;
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {this->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    ret = vkQueuePresentKHR(this->graphicsQueue, &presentInfo);

    if (ret != VK_SUCCESS) {
        if (ret != VK_ERROR_OUT_OF_DATE_KHR) {
            logError("Failed to Present Swap Chain Image!");
        }
        this->requiresRenderUpdate = true;
        return;
    }

    this->currentFrame = (this->currentFrame + 1) % this->imageCount;
}


void Renderer::addDeltaTime(const std::chrono::high_resolution_clock::time_point now, const float deltaTime) {
    this->accumulatedDeltaTime += static_cast<uint64_t>(deltaTime);
    this->lastDeltaTime = deltaTime;
    this->deltaTimes.push_back(deltaTime);

    std::chrono::duration<double, std::milli> timeSinceLastFrameRateUpdate = now - this->lastFrameRateUpdate;

    if (timeSinceLastFrameRateUpdate.count() >= 1000) {
        double deltaTimeAccumulated = 0;
        for (uint16_t i=0;i<this->deltaTimes.size(); i++) {
            deltaTimeAccumulated += this->deltaTimes[i];
        }

        this->frameRate = static_cast<uint16_t>((1000 / (deltaTimeAccumulated / this->deltaTimes.size())) * (timeSinceLastFrameRateUpdate.count() / 1000));
        this->lastFrameRateUpdate = now;
        this->deltaTimes.clear();
    }
}

float Renderer::getDeltaTime() const {
    return this->lastDeltaTime;
}

uint16_t Renderer::getFrameRate() const {
    return this->frameRate;
}

bool Renderer::doesShowWireFrame() const {
    return this->showWireFrame;
}

void Renderer::setShowWireFrame(bool showWireFrame) {
    this->showWireFrame = showWireFrame;
    this->requiresRenderUpdate = true;
}

VkRenderPass Renderer::getRenderPass() const {
    return this->renderPass;
}

VkExtent2D Renderer::getSwapChainExtent() const {
    return this->swapChainExtent;
}

bool Renderer::recreateRenderer() {
    if (!this->isReady()) {
        logError("Renderer has not been initialized!");
        return false;
    }

    this->destroySwapChainObjects();

    this->requiresRenderUpdate = false;

    if (!this->createSwapChain()) return false;
    if (!this->createRenderPass()) return false;

    for (Pipeline * pipeline : this->pipelines) {
        if (pipeline != nullptr) {
            if (!pipeline->createPipeline()) continue;
        }
    }

    if (!this->createDepthResources()) return false;
    if (!this->createFramebuffers()) return false;

    if (!this->createCommandBuffers()) return false;

    return true;
}

uint32_t Renderer::getImageCount() const {
    return this->imageCount;
}

void Renderer::forceRenderUpdate() {
    this->requiresRenderUpdate = true;
}

bool Renderer::isPaused() {
    return this->paused;
}

void Renderer::pause() {
    this->paused = true;

    vkQueueWaitIdle(this->graphicsQueue);
}

void Renderer::resume() {
    this->paused = false;
}

bool Renderer::isMaximized() {
    return this->maximized;
}

bool Renderer::isFullScreen() {
    return this->fullScreen;
}

Renderer::~Renderer() {
    logInfo("Destroying Renderer...");
    this->destroyRendererObjects();

    logInfo("Destroying Logical Device...");
    if (this->logicalDevice != nullptr) {
        vkDestroyDevice(this->logicalDevice, nullptr);
    }
    logInfo("Destroyed Renderer");
}

uint32_t Renderer::getGraphicsQueueIndex() const {
    return this->graphicsQueueIndex;
}

const GraphicsContext * Renderer::getGraphicsContext() const {
    return this->graphicsContext;
}

const VkPhysicalDeviceMemoryProperties & Renderer::getMemoryProperties() const {
    return this->memoryProperties;
}

void Renderer::setClearValue(const VkClearColorValue & clearColorValue) {
    this->clearValue = clearColorValue;
}

uint64_t Renderer::getAccumulatedDeltaTime() {
    return this->accumulatedDeltaTime;
}

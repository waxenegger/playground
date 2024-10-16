#include "includes/pipeline.h"

Renderer::Renderer(const GraphicsContext * graphicsContext, const VkPhysicalDevice & physicalDevice, const int & graphicsQueueIndex, const int & computeQueueIndex) :
    graphicsContext(graphicsContext), physicalDevice(physicalDevice), graphicsQueueIndex(graphicsQueueIndex), computeQueueIndex(computeQueueIndex) {

    const bool hasSeparateComputeQueue = this->computeQueueIndex != -1 && this->computeQueueIndex != this->graphicsQueueIndex;

    const uint16_t numberOfGraphicsQueues = this->graphicsContext->getNumberOfQueues(this->physicalDevice, this->graphicsQueueIndex);
    const bool hasAltQueue = numberOfGraphicsQueues > 1;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    const float priorities[] = {1.0f,1.0f};

    VkDeviceQueueCreateInfo queueCreateInfo {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.flags = 0;
    queueCreateInfo.pNext = nullptr;
    queueCreateInfo.queueFamilyIndex = this->graphicsQueueIndex;
    queueCreateInfo.queueCount = hasAltQueue ? 2 : 1;
    queueCreateInfo.pQueuePriorities = &priorities[0];

    queueCreateInfos.push_back(queueCreateInfo);

    if (hasSeparateComputeQueue) {
        queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.flags = 0;
        queueCreateInfo.pNext = nullptr;
        queueCreateInfo.queueFamilyIndex = this->computeQueueIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &priorities[1];

        queueCreateInfos.push_back(queueCreateInfo);
    }

    std::vector<const char * > extensionsToEnable = {
        "VK_KHR_swapchain",
        //"VK_KHR_shader_non_semantic_info"
    };
    if (this->useGpuCulling) {
        extensionsToEnable.push_back("VK_KHR_shader_draw_parameters");
    }

    if (this->graphicsContext->doesPhysicalDeviceSupportExtension(this->physicalDevice, "VK_EXT_memory_budget")) {
        extensionsToEnable.push_back("VK_EXT_memory_budget");
        this->memoryBudgetExtensionSupported = true;
    } else {
        logError("Your graphics card does not support VK_EXT_memory_budget! GPU memory usage has to be manually tracked!");
    }

    if (this->graphicsContext->doesPhysicalDeviceSupportExtension(this->physicalDevice, "VK_EXT_descriptor_indexing")) {
        extensionsToEnable.push_back("VK_EXT_descriptor_indexing");
        this->descriptorIndexingSupported = true;
    } else {
        logError("Your graphics card does not support VK_EXT_descriptor_indexing!");
    }

    VkPhysicalDeviceFeatures2 deviceFeatures { };
    deviceFeatures.sType =  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.features.samplerAnisotropy = VK_TRUE;
    deviceFeatures.features.multiDrawIndirect = this->useGpuCulling ? VK_TRUE : VK_FALSE;
    deviceFeatures.features.fillModeNonSolid = VK_TRUE;
    deviceFeatures.features.geometryShader = VK_TRUE;

    VkPhysicalDeviceVulkan12Features vulkan12Features { };
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.drawIndirectCount = true;
    vulkan12Features.descriptorIndexing = this->descriptorIndexingSupported;
    vulkan12Features.runtimeDescriptorArray = this->descriptorIndexingSupported;

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

    this->setPhysicalDeviceProperties();

    vkGetDeviceQueue(this->logicalDevice, this->graphicsQueueIndex , 0, &this->graphicsQueue);
    vkGetDeviceQueue(this->logicalDevice, this->graphicsQueueIndex , hasAltQueue ? 1 : 0, &this->altGraphicsQueue);
    vkGetDeviceQueue(this->logicalDevice, this->computeQueueIndex , 0, &this->computeQueue);
}

uint64_t Renderer::getPhysicalDeviceProperty(const std::string prop) const {
    if (this->physicalDevice == nullptr) return 0;

    const auto p = this->deviceProperties.find(prop);
    if (p == this->deviceProperties.end()) return 0;

    return p->second;
}

void Renderer::trackDeviceLocalMemory(const VkDeviceSize & delta,  const bool & isFree) {
    if (this->physicalDevice == nullptr || this->memoryBudgetExtensionSupported) return;

    const VkDeviceSize & total = this->getPhysicalDeviceProperty(DEVICE_MEMORY_LIMIT);
    VkDeviceSize use = this->getPhysicalDeviceProperty(DEVICE_MEMORY_USAGE_MANUALLY_TRACKED);
    if (isFree) {
        use = delta > use ? 0 : use - delta;
    } else {
        use = use + delta > total ? total : use + delta;
    }
    this->deviceProperties[DEVICE_MEMORY_USAGE_MANUALLY_TRACKED] = use;
}

void Renderer::setPhysicalDeviceProperties() {
    if (this->physicalDevice == nullptr) return;

    vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &this->memoryProperties);

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(this->physicalDevice, &properties);

    this->deviceProperties[UNIFORM_BUFFER_LIMIT] = properties.limits.maxUniformBufferRange;
    this->deviceProperties[STORAGE_BUFFER_LIMIT] = properties.limits.maxStorageBufferRange;
    this->deviceProperties[PUSH_CONSTANTS_LIMIT] = properties.limits.maxPushConstantsSize;
    this->deviceProperties[ALLOCATION_LIMIT] = properties.limits.maxMemoryAllocationCount;
    this->deviceProperties[COMPUTE_SHARED_MEMORY_LIMIT] = properties.limits.maxComputeSharedMemorySize;
    this->deviceProperties[DEVICE_MEMORY_LIMIT] = 0;
    this->deviceProperties[DEVICE_MEMORY_INDEX] = 0;

    for ( uint32_t j = 0; j < this->memoryProperties.memoryHeapCount; j++ ) {
        if ((memoryProperties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            this->deviceProperties[DEVICE_MEMORY_LIMIT] = memoryProperties.memoryHeaps[j].size;
            this->deviceProperties[DEVICE_MEMORY_INDEX] = j;
            break;
        }
    }

    logInfo("Some Physical Device Properties...");
    for (auto & entry : this->deviceProperties) {
        if (entry.first == DEVICE_MEMORY_INDEX) continue;
        logInfo(entry.first + ": " + Helper::formatMemoryUsage(entry.second));
    }
}

DeviceMemoryUsage Renderer::getDeviceMemory() const {
    DeviceMemoryUsage mem = { 0, 0, 0};

    if (this->physicalDevice == nullptr) return mem;

    if (!this->memoryBudgetExtensionSupported) {
        const VkDeviceSize & total = this->getPhysicalDeviceProperty(DEVICE_MEMORY_LIMIT);
        const VkDeviceSize & use = this->getPhysicalDeviceProperty(DEVICE_MEMORY_USAGE_MANUALLY_TRACKED);

        mem.total = total;
        mem.used = use;
        if (use < total) mem.available = total - use;

        return mem;
    }

    VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudgetExt;
    memoryBudgetExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
    memoryBudgetExt.pNext = nullptr;

    VkPhysicalDeviceMemoryProperties2 memPropsExtended;
    memPropsExtended.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    memPropsExtended.pNext = &memoryBudgetExt;
    memPropsExtended.memoryProperties = this->memoryProperties;

    vkGetPhysicalDeviceMemoryProperties2(this->physicalDevice, &memPropsExtended);
    if (memPropsExtended.pNext == nullptr) return mem;

    VkPhysicalDeviceMemoryBudgetPropertiesEXT * memoryBudget = static_cast<VkPhysicalDeviceMemoryBudgetPropertiesEXT *>(memPropsExtended.pNext);
    const uint64_t deviceLocalMemoryIndex = this->getPhysicalDeviceProperty(DEVICE_MEMORY_INDEX);

    if (memoryBudget->heapUsage[deviceLocalMemoryIndex] > memoryBudget->heapBudget[deviceLocalMemoryIndex]) return mem;

    mem.total = memoryBudget->heapBudget[deviceLocalMemoryIndex];
    mem.used = memoryBudget->heapUsage[deviceLocalMemoryIndex];
    mem.available = mem.total - mem.used;

    return mem;
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

    VkDeviceSize bufferSize = sizeof(GraphicsUniforms);

    bool initialized = true;

    this->uniformBuffer = std::vector<Buffer>(this->imageCount);
    for (auto & b : this->uniformBuffer) {
        b.createSharedUniformBuffer(this->getPhysicalDevice(), this->getLogicalDevice(), bufferSize);
        if (!b.isInitialized()) {
            initialized = false;
        }
    }

    bufferSize = sizeof(CullUniforms);

    this->uniformBufferCompute = std::vector<Buffer>(this->imageCount);
    for (auto & b : this->uniformBufferCompute) {
        b.createSharedUniformBuffer(this->getPhysicalDevice(), this->getLogicalDevice(), bufferSize);
        if (!b.isInitialized()) {
            initialized = false;
        }
    }

    return initialized;
}

void Renderer::updateUniformBuffers(int index) {
    const glm::vec4 pos(Camera::INSTANCE()->getPosition(), 1.0f);

    GraphicsUniforms graphUniforms {};
    graphUniforms.globalLightColorAndGlossiness = Renderer::SUN_COLOR_AND_GLOSS;
    graphUniforms.globalLightLocationAndStrength = Renderer::SUN_LOCATION_STRENGTH;
    graphUniforms.camera = pos;
    graphUniforms.viewProjMatrix = Camera::INSTANCE()->getProjectionMatrix() * Camera::INSTANCE()->getViewMatrix();
    memcpy(this->uniformBuffer[index].getBufferData(), &graphUniforms, sizeof(GraphicsUniforms));

    if (this->useGpuCulling) {
        CullUniforms cullUniforms {};
        cullUniforms.frustumPlanes = Camera::INSTANCE()->calculateFrustum(graphUniforms.viewProjMatrix);
        memcpy(this->uniformBufferCompute[index].getBufferData(), &cullUniforms, sizeof(CullUniforms));
    }
}

const Buffer & Renderer::getUniformBuffer(int index) const {
    return this->uniformBuffer[index];
}

const Buffer & Renderer::getUniformComputeBuffer(int index) const {
    return this->uniformBufferCompute[index];
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
        this->isReady() && this->swapChain != nullptr && this->swapChainImages.size() == this->imageCount &&
        this->imageAvailableSemaphores.size() == this->imageCount && this->renderFinishedSemaphores.size() == this->imageCount && this->inFlightFences.size() == this->imageCount &&
        this->swapChainFramebuffers.size() == this->swapChainImages.size() && this->depthImages.size() == this->swapChainImages.size() && this->depthImages.size() == this->imageCount &&
        this->graphicsCommandPool.isInitialized();
    if (!graphicCanRender) return false;

    if (!this->useGpuCulling) return true;

    return this->computeFinishedSemaphores.size() == this->imageCount && this->computeFences.size() == this->imageCount &&
            this->computeCommandPool.isInitialized() && this->indirectDrawBuffer[0].isInitialized();;
}


bool Renderer::addPipeline(std::unique_ptr<Pipeline> & pipeline, const int index) {
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

    if (index < 0) {
        this->pipelines.push_back(pipeline.release());
    } else {
        this->pipelines.insert(this->pipelines.begin() + index, pipeline.release());
    }

    if (!wasPaused) {
        this->forceRenderUpdate();
        this->resume();
    }

    return true;
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

    if (idx != -1) {
        auto & p = this->pipelines[idx];
        if (p != nullptr) {
            delete p;
            this->pipelines[idx] = nullptr;
        }

        this->pipelines.erase(this->pipelines.begin() + idx);
    }

    if (!wasPaused) {
        this->forceRenderUpdate();
        this->resume();
    }
}

bool Renderer::createRenderPass0(VkRenderPass & renderPass, const VkImageLayout initialLayout, const VkImageLayout depthImageFinalLayout, const bool clear)
{
    if (!this->isReady()) {
        logError("Renderer has not been initialized!");
        return false;
    }

    std::vector<VkAttachmentDescription> attachments;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = SWAP_CHAIN_IMAGE_FORMAT.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    if (clear) colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = initialLayout;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments.emplace_back(colorAttachment);

    VkFormat depthFormat;
    if (!GraphicsContext::findDepthFormat(this->physicalDevice, depthFormat)) {
        logError("Failed to Find Depth Format!");
    }

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    if (clear) depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = initialLayout;
    depthAttachment.finalLayout = depthImageFinalLayout;

    attachments.emplace_back(depthAttachment);

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::vector<VkSubpassDependency> dependencies {
        {
            VK_SUBPASS_EXTERNAL,
            0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,0
        }
    };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = dependencies.size();
    renderPassInfo.pDependencies = dependencies.data();

    VkResult ret = vkCreateRenderPass(this->logicalDevice, &renderPassInfo, nullptr, &renderPass);
    if (ret != VK_SUCCESS) {
       logError("Failed to Create Render Pass!");
       return false;
    }

    return true;
}

bool Renderer::createRenderPass() {
    return this->createRenderPass0(this->renderPass);
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

    const VkExtent2D & windowExtent = this->graphicsContext->getSwapChainExtent(surfaceCapabilities);
    if (windowExtent.width == 0 && windowExtent.height == 0) {
        // on windows this means we are minimized
        this->minimized = true;
        this->pause();
        return false;
    }
    this->swapChainExtent = windowExtent;

    this->maximized = SDL_GetWindowFlags(this->graphicsContext->getSdlWindow()) & SDL_WINDOW_MAXIMIZED;
    this->fullScreen =SDL_GetWindowFlags(this->graphicsContext->getSdlWindow()) & SDL_WINDOW_FULLSCREEN;

    if (this->imageCount == 0) {
        this->imageCount = DEFAULT_BUFFERING;
        if (this->imageCount > surfaceCapabilities.maxImageCount && surfaceCapabilities.maxImageCount != 0) this->imageCount = surfaceCapabilities.maxImageCount;
    }

    logInfo("Min/Max Buffering: " + std::to_string(surfaceCapabilities.minImageCount) + "/" + std::to_string(surfaceCapabilities.maxImageCount));

    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT & VK_IMAGE_USAGE_TRANSFER_SRC_BIT == 0) {
        logInfo("Swap Chain Recording is not supported");
    } else {
        this->swapChainRecordingSupported = true;
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
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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
        logError("Failed to Create Swap Chain: " + std::to_string(ret));
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
    this->computeFences.resize(this->imageCount);
    this->computeFinishedSemaphores.resize(this->imageCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < this->imageCount; i++) {

        if (vkCreateSemaphore(this->logicalDevice, &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(this->logicalDevice, &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(this->logicalDevice, &semaphoreInfo, nullptr, &this->computeFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(this->logicalDevice, &fenceInfo, nullptr, &this->inFlightFences[i]) != VK_SUCCESS ||
            vkCreateFence(this->logicalDevice, &fenceInfo, nullptr, &this->computeFences[i]) != VK_SUCCESS) {
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
    if (!this->graphicsCommandPool.isInitialized()) return false;

    if (!this->useGpuCulling) return true;

    this->computeCommandPool.create(this->getLogicalDevice(), this->getComputeQueueIndex());

    return this->computeCommandPool.isInitialized();
}

const CommandPool & Renderer::getGraphicsCommandPool() const {
    return this->graphicsCommandPool;
}

VkQueue Renderer::getGraphicsQueue() const {
    return this->graphicsQueue;
}

VkQueue Renderer::getAltGraphicsQueue() const {
    return this->altGraphicsQueue;
}

VkQueue Renderer::getComputeQueue() const {
    return this->computeQueue;
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
         framebufferInfo.width = this->swapChainExtent.width;
         framebufferInfo.height = this->swapChainExtent.height;
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
        ImageConfig conf;
        conf.format = depthFormat;
        conf.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        conf.width = this->swapChainExtent.width;
        conf.height = this->swapChainExtent.height;


        this->depthImages[i].createImage(this->getPhysicalDevice(), this->getLogicalDevice(), conf);

        if (!this->depthImages[i].isInitialized()) {
            logError("Failed to create Depth Image!");
            return false;
        }
    }

    return true;
}

bool Renderer::initRenderer() {
    if (!this->createRenderer() ||
        !this->createCommandPools() ||
        !this->createUniformBuffers()) return false;

    if (this->useGpuCulling) {
        if (!this->createIndirectDrawBuffers()) return false;
    }

    GlobalTextureStore::INSTANCE()->uploadTexturesToGPU(this);

    return true;
}

void Renderer::setIndirectDrawBufferSize(const VkDeviceSize & size) {
    this->indirectDrawBufferSize = size;
}

void Renderer::destroyRendererObjects() {
    if (this->logicalDevice == nullptr) return;

    this->destroySwapChainObjects();

    for (auto & b : this->uniformBuffer) {
        b.destroy(this->logicalDevice);
    }

    for (auto & b : this->uniformBufferCompute) {
        b.destroy(this->logicalDevice);
    }

    for (auto & b : this->indirectDrawBuffer) {
        b.destroy(this->logicalDevice);
    }

    for (auto & b : this->indirectDrawCountBuffer) {
        b.destroy(this->logicalDevice);
    }

    for (Pipeline * pipeline : this->pipelines) {
        if (pipeline != nullptr) {
            delete pipeline;
            pipeline = nullptr;
        }
    }
    pipelines.clear();

    this->graphicsCommandPool.destroy(this->logicalDevice);
    this->computeCommandPool.destroy(this->logicalDevice);

    GlobalTextureStore::INSTANCE()->cleanUpTextures(this->logicalDevice);
}

void Renderer::destroySyncObjects() {
    if (this->logicalDevice == nullptr) return;

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

        if (i < this->computeFinishedSemaphores.size()) {
            if (this->computeFinishedSemaphores[i] != nullptr) {
                vkDestroySemaphore(this->logicalDevice, this->computeFinishedSemaphores[i], nullptr);
            }
        }

        if (i < this->inFlightFences.size()) {
            if (this->inFlightFences[i] != nullptr) {
                vkDestroyFence(this->logicalDevice, this->inFlightFences[i], nullptr);
            }
        }

        if (i < this->computeFences.size()) {
            if (this->computeFences[i] != nullptr) {
                vkDestroyFence(this->logicalDevice, this->computeFences[i], nullptr);
            }
        }
    }

    this->renderFinishedSemaphores.clear();
    this->imageAvailableSemaphores.clear();
    this->computeFinishedSemaphores.clear();
    this->inFlightFences.clear();
    this->computeFences.clear();
}

void Renderer::destroySwapChainObjects(const bool destroyPipelines) {
    if (this->logicalDevice == nullptr) return;

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

    if (destroyPipelines) {
        for (Pipeline * pipeline : this->pipelines) {
            if (pipeline != nullptr) {
                pipeline->destroyPipeline();
            }
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

    for (uint16_t j=0;j<this->cachedFrames.size();j++) {
        this->cachedFrames[j]->destroy(this->logicalDevice);
    }
    this->cachedFrames.clear();

    if (this->swapChain != nullptr) {
        vkDestroySwapchainKHR(this->logicalDevice, this->swapChain, nullptr);
        this->swapChain = nullptr;
    }

    this->destroySyncObjects();
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

void Renderer::renderPipeline(const std::string pipelineName, const VkRenderPass renderPass, const VkCommandBuffer & commandBuffer, const uint16_t imageIndex)
{
    Pipeline * pipeline = this->getPipeline(pipelineName);
    if (pipeline == nullptr || !this->isReady() || !pipeline->canRender()) return;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = this->swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = this->swapChainExtent;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    static_cast<GraphicsPipeline *>(pipeline)->update();
    static_cast<GraphicsPipeline *>(pipeline)->draw(commandBuffer, imageIndex);

    vkCmdEndRenderPass(commandBuffer);
}

/**
 *  Central method that creates the render pass and records all instructions in the commandbuffer
 *  iterating over all pipelines created in the list.
 *  Special care has been taken for the compute culling + indirect GPU draw case
 *  which requires memory barriers when the compute queue and graphics queue are not part of the same physical queue
 */
VkCommandBuffer Renderer::createCommandBuffer(const uint16_t commandBufferIndex, const uint16_t imageIndex, const bool useSecondaryCommandBuffers) {
    const VkCommandBuffer & commandBuffer = this->graphicsCommandPool.beginPrimaryCommandBuffer(this->logicalDevice);
    if (commandBuffer == nullptr) return nullptr;

    if (this->useGpuCulling && this->getGraphicsQueueIndex() != this->getComputeQueueIndex()) {
        for (Pipeline * pipeline : this->pipelines) {
            if (pipeline->isEnabled() && isReady() && pipeline->canRender()) {
                const int indIndex = static_cast<GraphicsPipeline *>(pipeline)->getIndirectBufferIndex();
                if (indIndex < 0) continue;

                const std::array<VkBufferMemoryBarrier,2> indirectBarriers {{
                {
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    nullptr,
                    VK_ACCESS_SHADER_WRITE_BIT,
                    VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                    static_cast<uint32_t>(this->getComputeQueueIndex()),
                    static_cast<uint32_t>(this->getGraphicsQueueIndex()),
                    this->indirectDrawBuffer[indIndex].getBuffer(),
                    0,
                    this->indirectDrawBuffer[indIndex].getSize()
                },
                {
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    nullptr,
                    VK_ACCESS_SHADER_WRITE_BIT,
                    VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                    static_cast<uint32_t>(this->getComputeQueueIndex()),
                    static_cast<uint32_t>(this->getGraphicsQueueIndex()),
                    this->indirectDrawCountBuffer[indIndex].getBuffer(),
                    0,
                    this->indirectDrawCountBuffer[indIndex].getSize()
                }
                }};

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                    0,
                    0, nullptr,
                    indirectBarriers.size(),
                    indirectBarriers.data(),
                    0, nullptr
                );
            }
        }
    }

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
        if (pipeline->isEnabled() && this->isReady() && pipeline->canRender()) {
            static_cast<GraphicsPipeline *>(pipeline)->update();
            static_cast<GraphicsPipeline *>(pipeline)->draw(commandBuffer, commandBufferIndex);
        }
    }

    vkCmdEndRenderPass(commandBuffer);

    this->graphicsCommandPool.endCommandBuffer(commandBuffer);

    return commandBuffer;
}

bool Renderer::createCommandBuffers() {
    this->commandBuffers.resize(this->swapChainFramebuffers.size());
    this->computeBuffers.resize(this->swapChainFramebuffers.size());

    this->lastFrameRateUpdate = std::chrono::high_resolution_clock::now();

    return true;
}

void Renderer::render(const bool addFrameToCache) {
    if (this->requiresRenderUpdate) {
        this->waitForQueuesToBeIdle();

        bool success = true;
        if (this->requiresSwapChainRecreate) {
            success = this->createRenderer();
            if (success) {
                const VkExtent2D & windowSize = this->getSwapChainExtent();
                Camera::INSTANCE()->setAspectRatio(static_cast<float>(windowSize.width) / windowSize.height);
            }
        } else {
            this->recreatePipelines();
        }

        this->waitForQueuesToBeIdle();

        this->resetRenderUpdate();
    }

    if (this->paused) {
        if (this->renderCachedFrame()) return;

        this->resume();
        this->waitForQueuesToBeIdle();

        if (this->requiresRenderUpdate) return;
    }

    if (this->uploadTexturesToGPU) {
        this->uploadTexturesToGPU = false;
        if (GlobalTextureStore::INSTANCE()->uploadTexturesToGPU(this) > 0) return;
    }

    if (this->useGpuCulling) {
        this->computeFrame();
    } else {
        GlobalRenderableStore::INSTANCE()->performFrustumCulling(Camera::INSTANCE()->getFrustumPlanes());
        this->updateUniformBuffers(this->currentFrame);
    }

    this->renderFrame(addFrameToCache);
}

/**
 * Renders stored frame at given index
 * which comes in handy on minimization and for debugging
 */
bool Renderer::renderCachedFrame()
{
    if (!this->swapChainRecordingSupported || this->cachedFrames.empty()) return false;

    uint32_t frameIndex = this->cachedFrameIndex;
    if (frameIndex >= this->cachedFrames.size()) frameIndex = 0;
    if (!this->cachedFrames[frameIndex]->isInitialized()) {
        this->forceRenderUpdate(true);
        return false;
    }

    uint32_t imageIndex;
    VkResult ret = vkAcquireNextImageKHR(this->logicalDevice, this->swapChain, UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (ret != VK_SUCCESS) {
        if (ret != VK_ERROR_OUT_OF_DATE_KHR && ret != VK_SUBOPTIMAL_KHR) logError("Failed at graphics vkAcquireNextImageKHR");
        this->forceRenderUpdate(true);
        return false;
    }

    auto & tmpImage = this->swapChainImages[imageIndex];
    const auto swapChainImageExtent = this->getSwapChainExtent();
    if (!tmpImage.isInitialized()) {
        this->forceRenderUpdate(true);
        return false;
    }

    VkRenderPass cachedFrameRenderPass {};
    if (!this->createRenderPass0(cachedFrameRenderPass, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, false)) {
        this->forceRenderUpdate(true);
        return false;
    }

    const VkCommandBuffer & copyFrameCommandBuffer = this->graphicsCommandPool.beginPrimaryCommandBuffer(logicalDevice);
    tmpImage.transitionImageLayout(copyFrameCommandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    tmpImage.copyBufferToImage(copyFrameCommandBuffer, this->cachedFrames[frameIndex]->getBuffer(), swapChainImageExtent.width, swapChainImageExtent.height, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    this->renderPipeline(GUI_PIPELINE, cachedFrameRenderPass, copyFrameCommandBuffer, imageIndex);
    this->graphicsCommandPool.endCommandBuffer(copyFrameCommandBuffer);
    this->graphicsCommandPool.submitCommandBuffer(this->logicalDevice, this->graphicsQueue, copyFrameCommandBuffer);
    vkDestroyRenderPass(this->logicalDevice, cachedFrameRenderPass, nullptr);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSwapchainKHR swapChains[] = {this->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    ret = vkQueuePresentKHR(this->graphicsQueue, &presentInfo);
    if (ret != VK_SUCCESS) {
        if (ret != VK_ERROR_OUT_OF_DATE_KHR && ret != VK_SUBOPTIMAL_KHR) logError("Failed at graphics vkQueuePresentKHR");
        this->forceRenderUpdate(true);
        return false;
    }

    return true;
}

/**
 *  Central method for cpu culling and indirect draw buffer population
 */
void Renderer::computeFrame() {
    VkResult ret = vkWaitForFences(this->logicalDevice, 1, &this->computeFences[this->currentFrame], VK_TRUE, UINT64_MAX);
    if (ret != VK_SUCCESS) {
        return;
    }

    ret = vkResetFences(this->logicalDevice, 1, &this->computeFences[this->currentFrame]);
    if (ret != VK_SUCCESS) {
        logError("Failed to Reset Fence!");
    }

    if (this->computeBuffers[this->currentFrame] != nullptr) {
        this->computeCommandPool.freeCommandBuffer(this->logicalDevice, this->computeBuffers[this->currentFrame]);
    }

    const VkCommandBuffer & commandBuffer = this->computeCommandPool.beginPrimaryCommandBuffer(this->logicalDevice);
    if (commandBuffer == nullptr) return;

    this->computeBuffers[this->currentFrame] = commandBuffer;

    for (Pipeline * pipeline : this->pipelines) {
        if (pipeline->isEnabled() && isReady() && !pipeline->canRender()) {
            ComputePipeline * compPipe = static_cast<ComputePipeline *>(pipeline);
            const int ind = compPipe->getIndirectBufferIndex();

            if (this->getGraphicsQueueIndex() != this->getComputeQueueIndex()) {
                const std::array<VkBufferMemoryBarrier,2> indirectBarriers {{
                    {
                        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                        nullptr,
                        VK_ACCESS_SHADER_WRITE_BIT,
                        VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                        static_cast<uint32_t>(this->getComputeQueueIndex()),
                        static_cast<uint32_t>(this->getGraphicsQueueIndex()),
                        this->indirectDrawBuffer[ind].getBuffer(),
                        0,
                        this->indirectDrawBuffer[ind].getSize()
                    },
                    {
                        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                        nullptr,
                        VK_ACCESS_SHADER_WRITE_BIT,
                        VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                        static_cast<uint32_t>(this->getComputeQueueIndex()),
                        static_cast<uint32_t>(this->getGraphicsQueueIndex()),
                        this->indirectDrawCountBuffer[ind].getBuffer(),
                        0,
                        this->indirectDrawCountBuffer[ind].getSize()
                    }
                }};

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                    0,
                    0, nullptr,
                    indirectBarriers.size(),
                    indirectBarriers.data(),
                    0, nullptr
                );
            }

            compPipe->update();
            compPipe->compute(commandBuffer, this->currentFrame);
        }
    }

    this->computeCommandPool.endCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.commandBufferCount = this->computeBuffers.empty() ? 0 : 1;
    submitInfo.pCommandBuffers = &this->computeBuffers[this->currentFrame];

    VkSemaphore signalSemaphores[] = {this->computeFinishedSemaphores[this->currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    this->updateUniformBuffers(this->currentFrame);

    ret = vkQueueSubmit(this->computeQueue, 1, &submitInfo, this->computeFences[this->currentFrame]);
    if (ret != VK_SUCCESS) {
        logError("Failed to Submit Compute Command Buffer!");
        return;
    }
}

void Renderer::renderFrame(const bool addFrameToCache) {
    VkResult ret = vkWaitForFences(this->logicalDevice, 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
    if (ret != VK_SUCCESS) {
        logError("Failed at graphics vkWaitForFences");
        this->forceRenderUpdate(true);
        return;
    }

    ret = vkResetFences(this->logicalDevice, 1, &this->inFlightFences[this->currentFrame]);
    if (ret != VK_SUCCESS) {
        logError("Failed at graphics vkResetFences");
        this->forceRenderUpdate(true);
        return;
    }

    uint32_t imageIndex;
    ret = vkAcquireNextImageKHR(this->logicalDevice, this->swapChain, UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (ret != VK_SUCCESS) {
        if (ret != VK_ERROR_OUT_OF_DATE_KHR && ret != VK_SUBOPTIMAL_KHR) logError("Failed at graphics vkAcquireNextImageKHR");
        this->forceRenderUpdate(true);
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

    if (this->useGpuCulling) {
        waitSemaphores.push_back(this->computeFinishedSemaphores[this->currentFrame]);
    }

    std::vector<VkPipelineStageFlags> waitStages = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    if (this->useGpuCulling) {
        waitStages.push_back(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }

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
        logError("Failed at graphics vkQueueSubmit");
        this->forceRenderUpdate(true);
        return;
    }

    if (addFrameToCache) this->addFrameToCache(imageIndex);

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
        if (ret != VK_ERROR_OUT_OF_DATE_KHR && ret != VK_SUBOPTIMAL_KHR) logError("Failed at graphics vkQueuePresentKHR");
        this->forceRenderUpdate(true);
        return;
    }

    this->currentFrame = (this->currentFrame + 1) % this->imageCount;
}

/**
 * Adds swapchain image for given image index to cache as a wrapped VkBuffer
 */
void Renderer::addFrameToCache(const uint32_t imageIndex)
{
    if (!this->recording) return;

    if (this->cachedFrames.size() >= FRAME_RECORDING_MAX_FRAMES) {
        this->setRecording(false);
        return;
    }

    auto & tmpImage = this->swapChainImages[imageIndex];
    const auto swapChainImageExtent = this->getSwapChainExtent();
    if (!tmpImage.isInitialized()) return;

    VkExtent3D copyExtent {};
    copyExtent.width = swapChainImageExtent.width;
    copyExtent.height = swapChainImageExtent.height;
    copyExtent.depth = 1;

    VkImageSubresourceLayers frameCopySubResourceLayers {};
    frameCopySubResourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    frameCopySubResourceLayers.layerCount = 1;

    VkBufferImageCopy frameCopy {};
    frameCopy.imageExtent = copyExtent;
    frameCopy.imageSubresource = frameCopySubResourceLayers;

    const VkDeviceSize frameCopyBufferSize = 4 * 4 * copyExtent.width * copyExtent.height;

    const auto memUsage = this->getDeviceMemory();
    if (memUsage.available < 2*frameCopyBufferSize) {
        this->setRecording(false);
        return;
    }

    auto cachedFrame = std::make_unique<Buffer>();
    cachedFrame->createBuffer(this->physicalDevice, this->logicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, frameCopyBufferSize);
    if (!cachedFrame->isInitialized()) return;

    this->cachedFrames.push_back(std::move(cachedFrame));
    this->trackDeviceLocalMemory(frameCopyBufferSize*1.2);

    const VkCommandBuffer & copyFrameCommandBuffer = this->graphicsCommandPool.beginPrimaryCommandBuffer(logicalDevice);

    tmpImage.transitionImageLayout(copyFrameCommandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkCmdCopyImageToBuffer(copyFrameCommandBuffer, tmpImage.getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, this->cachedFrames[this->cachedFrames.size()-1]->getBuffer(), 1, &frameCopy);
    tmpImage.transitionImageLayout(copyFrameCommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    this->graphicsCommandPool.endCommandBuffer(copyFrameCommandBuffer);
    this->graphicsCommandPool.submitCommandBuffer(this->logicalDevice, this->graphicsQueue, copyFrameCommandBuffer);}

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

std::vector<std::unique_ptr<Buffer>> & Renderer::getCachedFrames() {
    return this->cachedFrames;
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
    this->forceRenderUpdate();
}

bool Renderer::isMinimized() const
{
    return this->minimized;
}

bool Renderer::usesGpuCulling() const
{
    return this->useGpuCulling;
}

void Renderer::setGpuCulling(const bool useGpuCulling)
{
    this->useGpuCulling = useGpuCulling;
}

bool Renderer::isRecording() const
{
    return this->recording;
}

void Renderer::setRecording(const bool recording)
{
    const bool oldValue = this->recording;
    const bool newValue = this->swapChainRecordingSupported && recording;
    if (newValue && !oldValue) {
        for (auto & f : this->cachedFrames) f->destroy(this->logicalDevice);
        this->cachedFrames.clear();
    }

    this->recording = newValue;
}

VkRenderPass Renderer::getRenderPass() const {
    return this->renderPass;
}

VkExtent2D Renderer::getSwapChainExtent() const {
    return this->swapChainExtent;
}

std::vector<MemoryUsage> Renderer::getMemoryUsage() const {
    std::vector<MemoryUsage> memStats;

    MemoryUsage rendererMem;
    rendererMem.name = "renderer";

    for (auto & iB : this->indirectDrawBuffer) {
        rendererMem.indirectBufferTotal += iB.getSize();
        rendererMem.indirectBufferUsesDeviceLocal = this->usesDeviceIndirectDrawBuffer[0];
    }

    memStats.emplace_back(rendererMem);

    for (const auto & p : this->pipelines) {
        if (p->canRender()) {
            const auto & graphicsPipe = static_cast<GraphicsPipeline * >(p);
            MemoryUsage memUse = graphicsPipe->getMemoryUsage();
            memStats.emplace_back(memUse);
        } else {
            const auto & computePipe = static_cast<ComputePipeline * >(p);
            MemoryUsage memUse = computePipe->getMemoryUsage();
            memStats.emplace_back(memUse);
        }
    }

    return memStats;
}

bool Renderer::createRenderer(const bool recreatePipelines) {
    if (!this->isReady()) {
        logError("Renderer has not been initialized!");
        return false;
    }

    this->destroySwapChainObjects();

    if (!this->createSwapChain()) return false;
    if (!this->createSyncObjects()) return false;
    if (!this->createRenderPass()) return false;

    if (recreatePipelines) {
        for (Pipeline * pipeline : this->pipelines) {
            if (pipeline != nullptr) {
                if (!pipeline->createPipeline()) continue;
            }
        }
    }

    if (!this->createDepthResources()) return false;
    if (!this->createFramebuffers()) return false;

    if (!this->createCommandBuffers()) return false;

    return true;
}

bool Renderer::recreatePipelines() {
    if (!this->isReady()) {
        logError("Renderer has not been initialized!");
        return false;
    }

    for (Pipeline * pipeline : this->pipelines) {
        if (pipeline != nullptr) {
            pipeline->destroyPipeline();
            if (!pipeline->createPipeline()) continue;
        }
    }

    return true;
}

uint32_t Renderer::getImageCount() const {
    return this->imageCount;
}

void Renderer::forceRenderUpdate(const bool requiresSwapChainRecreate) {
    this->requiresRenderUpdate = true;
    this->requiresSwapChainRecreate = requiresSwapChainRecreate;
}

void Renderer::resetRenderUpdate() {
    this->requiresRenderUpdate = false;
    this->requiresSwapChainRecreate = false;
}

void Renderer::forceNewTexturesUpload() {
    this->uploadTexturesToGPU = true;
}

bool Renderer::isPaused() {
    return this->paused;
}

void Renderer::waitForQueuesToBeIdle() {
    if (this->logicalDevice == nullptr) return;

    if (this->useGpuCulling) {
        vkQueueWaitIdle(this->computeQueue);
    }

    vkQueueWaitIdle(this->graphicsQueue);
}

void Renderer::pause() {
    if (this->paused) return;

    SDL_SetWindowResizable(this->graphicsContext->getSdlWindow(), SDL_FALSE);
    this->paused = true;
    this->waitForQueuesToBeIdle();
}

void Renderer::resume() {
    this->paused = false;
    SDL_SetWindowResizable(this->graphicsContext->getSdlWindow(), SDL_TRUE);
}

bool Renderer::isMaximized() {
    return this->maximized;
}

bool Renderer::isFullScreen() {
    return this->fullScreen;
}

bool Renderer::hasConnectionToServer()
{
    return this->isConnectedToServer;
}

void Renderer::setIsConnectedToServer(const bool connected)
{
    if (this->isConnectedToServer && !connected) {
        Camera::INSTANCE()->linkToRenderable(nullptr);
    }

    this->isConnectedToServer = connected;
}

Renderer::~Renderer() {
    if (this->logicalDevice == nullptr) return;

    this->pause();

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

Buffer & Renderer::getIndirectDrawBuffer(const int & index) {
    if (index >= this->indirectDrawBuffer.size()) return this->indirectDrawBuffer[0];

    return this->indirectDrawBuffer[index];
}

Buffer & Renderer::getIndirectDrawCountBuffer(const int & index) {
    if (index >= this->indirectDrawCountBuffer.size()) return this->indirectDrawBuffer[0];

    return this->indirectDrawCountBuffer[index];
}

void Renderer::setMaxIndirectCallCount(uint32_t maxIndirectDrawCount, const int & index)
{
    if (index >= this->maxIndirectDrawCount.size()) return;

    this->maxIndirectDrawCount[index] = maxIndirectDrawCount;
}

uint32_t Renderer::getMaxIndirectCallCount(const int & index)
{
    if (index >= this->maxIndirectDrawCount.size()) return 0;

    return this->maxIndirectDrawCount[index];
}

uint32_t Renderer::getComputeQueueIndex() const {
    return this->computeQueueIndex;
}


/**
 *  For compute culling and indirect drawing we require a buffer for the draw commands
 *  as well as the count that is the draws left over after culling both of which will be
 *  populated by the compute shader
 */

bool Renderer::createIndirectDrawBuffers()
{
    const bool hasEnoughDeviceLocalSpace = this->getDeviceMemory().available >= this->indirectDrawBufferSize * INDIRECT_DRAW_DEFAULT_NUMBER_OF_BUFFERS;

    this->indirectDrawBuffer = std::vector<Buffer>(INDIRECT_DRAW_DEFAULT_NUMBER_OF_BUFFERS);
    this->indirectDrawCountBuffer = std::vector<Buffer>(INDIRECT_DRAW_DEFAULT_NUMBER_OF_BUFFERS);
    this->usesDeviceIndirectDrawBuffer = std::vector<bool>(INDIRECT_DRAW_DEFAULT_NUMBER_OF_BUFFERS, hasEnoughDeviceLocalSpace);
    this->maxIndirectDrawCount = std::vector<uint32_t>(INDIRECT_DRAW_DEFAULT_NUMBER_OF_BUFFERS, 0);

    VkResult result;
    int i=0;
    for (auto & b : this->indirectDrawBuffer) {
        result = b.createIndirectDrawBuffer(this->physicalDevice, this->logicalDevice, this->indirectDrawBufferSize, this->usesDeviceIndirectDrawBuffer[i]);

        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            this->usesDeviceIndirectDrawBuffer[i] = false;
            b.createIndirectDrawBuffer(this->physicalDevice, this->logicalDevice, this->indirectDrawBufferSize, this->usesDeviceIndirectDrawBuffer[i]);
        }

        if (!b.isInitialized()) return false;
        if (this->usesDeviceIndirectDrawBuffer[i]) this->trackDeviceLocalMemory(b.getSize());

        i++;
    }

    const VkDeviceSize countBufferSize = sizeof(uint32_t);
    bool useDeviceLocalMemory = this->getDeviceMemory().available >= countBufferSize * INDIRECT_DRAW_DEFAULT_NUMBER_OF_BUFFERS;

    for (auto & b : this->indirectDrawCountBuffer) {

        b.createIndirectDrawBuffer(this->physicalDevice, this->logicalDevice, countBufferSize, useDeviceLocalMemory);
        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            useDeviceLocalMemory = false;
            b.createIndirectDrawBuffer(this->physicalDevice, this->logicalDevice, countBufferSize, useDeviceLocalMemory);
        }

        if (!b.isInitialized()) return false;
        if (useDeviceLocalMemory) this->trackDeviceLocalMemory(b.getSize());
    }

    return true;
}

void Renderer::setCachedFrameIndex(const int frameIndex)
{
    this->cachedFrameIndex = frameIndex;
}

int Renderer::getNextIndirectBufferIndex()
{
    if (this->usedIndirectBufferCount + 1 >= INDIRECT_DRAW_DEFAULT_NUMBER_OF_BUFFERS) return -1;

    const int ret = this->usedIndirectBufferCount;
    this->usedIndirectBufferCount++;

    return ret;
}

glm::vec4 Renderer::SUN_COLOR_AND_GLOSS = {1.0f, 1.0f, 1.0f, 10.0f};
glm::vec4 Renderer::SUN_LOCATION_STRENGTH = {0.0f, 1000000.0f, 0.0f, 1.0f};

#include "includes/graphics.h"


void GraphicsContext::initWindow(const std::string & appName) {
    if (this->isWindowActive()) return;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        logError("Could not initialize SDL! Error: " + std::string(SDL_GetError()));
    }

    this->sdlWindow =
            SDL_CreateWindow(appName.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (this->sdlWindow == nullptr) {
        logError("SDL Window could not be created! Error: " + std::string(SDL_GetError()));
    }
}

void GraphicsContext::createVulkanInstance(const std::string & appName, const uint32_t version) {
    if (this->isVulkanActive()) return;
    
    VkApplicationInfo app;
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pNext = nullptr,
    app.pApplicationName = appName.c_str();
    app.applicationVersion = version;
    app.pEngineName = appName.c_str();
    app.engineVersion = version;
    app.apiVersion = VULKAN_VERSION;

    VkInstanceCreateInfo inst_info;
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = nullptr;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app;
    inst_info.enabledLayerCount = this->vulkanLayers.size();
    inst_info.ppEnabledLayerNames = !this->vulkanLayers.empty() ? this->vulkanLayers.data() : nullptr;
    inst_info.enabledExtensionCount = this->vulkanExtensions.size();
    inst_info.ppEnabledExtensionNames = !this->vulkanExtensions.empty() ? this->vulkanExtensions.data() : nullptr;

    const VkResult ret = vkCreateInstance(&inst_info, nullptr, &this->vulkanInstance);
    if (ret != VK_SUCCESS) {
        logError("Failed to create Vulkan Instance");
        return;
    }
    
    this->queryVulkanInstanceExtensions();
}

bool GraphicsContext::queryVulkanInstanceExtensions() {
    if (!this->isWindowActive()) return false;
    
    uint32_t extensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(this->sdlWindow, &extensionCount, nullptr) == SDL_FALSE) {
        logError("Could not get SDL Vulkan Extensions: " + std::string(SDL_GetError()));
        return false;
    } else {
        this->vulkanExtensions.resize(extensionCount);
        return SDL_Vulkan_GetInstanceExtensions(this->sdlWindow, &extensionCount, this->vulkanExtensions.data());
    }

    return true;
}

void GraphicsContext::initVulkan(const std::string & appName, const uint32_t version) {
    if (this->isVulkanActive() || !this->isWindowActive()) return;
    
    if (!this->queryVulkanInstanceExtensions()) return;
    this->listVulkanExtensions();

    this->createVulkanInstance(appName, version);
    if (this->vulkanInstance == nullptr) return;
    
    if (SDL_Vulkan_CreateSurface(this->sdlWindow, this->vulkanInstance, &this->vulkanSurface) == SDL_FALSE) {
        logError("Failed to Create Vulkan Surface: " + std::string(SDL_GetError()));
        this->quitVulkan();
    }
}

void GraphicsContext::initGraphics(const std::string & appName, const uint32_t version) {
    if (this->isGraphicsActive()) return;
    
    this->initWindow(appName);
    this->initVulkan(appName, version);

    this->queryPhysicalDevices();
}

bool GraphicsContext::isWindowActive() const {
    return this->sdlWindow != nullptr;
}

bool GraphicsContext::isVulkanActive() const {
    return this->vulkanInstance != nullptr && this->vulkanSurface != nullptr;
}

bool GraphicsContext::isGraphicsActive() const {
    return this->isWindowActive() && this->isVulkanActive();
}

void GraphicsContext::quitWindow() {
    if (this->sdlWindow != nullptr) {
        SDL_DestroyWindow(this->sdlWindow);
        this->sdlWindow = nullptr;
    }

    SDL_Quit();
}

void GraphicsContext::quitVulkan() {
    if (this->vulkanSurface != nullptr) {
        vkDestroySurfaceKHR(this->vulkanInstance, this->vulkanSurface, nullptr);
        this->vulkanSurface = nullptr;
    }

    if (this->vulkanInstance != nullptr) {
        vkDestroyInstance(this->vulkanInstance, nullptr);
        this->vulkanInstance = nullptr;
    }
}

void GraphicsContext::quitGraphics() {
    if (!this->isGraphicsActive()) return;

    logInfo("Shutting down Graphics Context...");

    this->quitVulkan();
    this->quitWindow();

    logInfo("Shut down Graphics Context");
}

void GraphicsContext::listVulkanExtensions() {
    if (this->vulkanExtensions.empty()) return;

    logInfo("Extensions:");
    for (auto & extension : this->vulkanExtensions) {
        logInfo("\t" + std::string(extension));
    }
}

void GraphicsContext::listLayerNames() {
    uint32_t layerCount = 0;
    VkResult ret;

    ret = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (ret != VK_SUCCESS) {
        logError("Failed to query Layer Properties!");
        return;
    }

    std::vector<VkLayerProperties> availableLayers(layerCount);

    ret = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    if (ret != VK_SUCCESS) {
        logError("Failed to query Layer Properties!");
        return;
    }

    if (availableLayers.empty()) return;

    logInfo("Layers:");
    for (auto & layer : availableLayers) {
        logInfo("\t" + std::string(layer.layerName));
    }
}

void GraphicsContext::listPhysicalDevices() {
    if (this->physicalDevices.empty()) return;

    VkPhysicalDeviceProperties physicalProperties;

    logInfo("Physical Devices:");
    for (auto & device : this->physicalDevices) {
        vkGetPhysicalDeviceProperties(device, &physicalProperties);
        logInfo("\t" + std::string(physicalProperties.deviceName) + 
                "\t[Type: " + std::to_string(physicalProperties.deviceType) + "]");
    }
}

const std::vector<VkExtensionProperties> GraphicsContext::queryDeviceExtensions(const VkPhysicalDevice & device) const {
    std::vector<VkExtensionProperties> extensions;
    
    if (!this->isGraphicsActive() || device == nullptr) return extensions;
    
    uint32_t deviceExtensionCount = 0;
    
    VkResult ret = vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);
    if (ret == VK_SUCCESS && deviceExtensionCount > 0) {
        extensions.resize(deviceExtensionCount);
        ret = vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, extensions.data());
        if (ret != VK_SUCCESS) {
            logError("Failed to query Device Extensions!");
        }
    }
    
    return extensions;
}

void GraphicsContext::queryPhysicalDevices() {
    if (!this->isGraphicsActive()) return;
    
    uint32_t physicalDeviceCount = 0;
    
    VkResult ret = vkEnumeratePhysicalDevices(this->vulkanInstance, &physicalDeviceCount, nullptr);
    if (ret != VK_SUCCESS) {
        logError("Failed to query Physical Devices!");
        return;
    }
    
    if (physicalDeviceCount ==0) {
        logError("No Physical Vulkan Device found!");
        return;
    }
    
    this->physicalDevices.resize(physicalDeviceCount);
    ret = vkEnumeratePhysicalDevices(this->vulkanInstance, &physicalDeviceCount, this->physicalDevices.data());
    if (ret != VK_SUCCESS) {
        logError("Failed to query Physical Devices!");
    }
}

const std::tuple<VkPhysicalDevice, int> GraphicsContext::pickBestPhysicalDeviceAndQueueIndex() {
    std::tuple<VkPhysicalDevice, int> choice = std::make_tuple(nullptr, -1);

    if (this->physicalDevices.empty()) {
        return choice;
    }

    int highestScore = 0;
    int i=0;
    for (auto & device : this->physicalDevices) {
        const std::tuple<int, int> scoreAndQueueIndex = this->ratePhysicalDevice(device);
        if (std::get<0>(scoreAndQueueIndex) > highestScore && std::get<1>(scoreAndQueueIndex) != -1) {
            highestScore = std::get<0>(scoreAndQueueIndex);
            choice = std::make_tuple(device, std::get<1>(scoreAndQueueIndex));
        }
        i++;
    }

    return choice;
}

bool GraphicsContext::doesPhysicalDeviceSupportExtension(const VkPhysicalDevice & device, const std::string extension) const {
    const std::vector<VkExtensionProperties> & availableExtensions = this->queryDeviceExtensions(device);

    for (auto & extProp : availableExtensions) {
        const std::string extName = std::string(extProp.extensionName);
        if (extName.compare(extension) == 0) {
            return true;
        }
    }

    return false;
}

const std::tuple<int,int> GraphicsContext::ratePhysicalDevice(const VkPhysicalDevice & device) {
    // check if physical device supports swap chains and required surface format
    if (!this->isGraphicsActive() || 
        !this->doesPhysicalDeviceSupportExtension(device, "VK_KHR_swapchain") ||
        !this->isPhysicalDeviceSurfaceFormatsSupported(device, SWAP_CHAIN_IMAGE_FORMAT)) {
            return std::make_tuple(0, -1);
    };

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;
    switch(deviceProperties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
            score = 1500;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score = 1000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score = 500;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score = 250;
            break;
        default:
            break;
    }

    if (deviceFeatures.geometryShader) {
        score += 5;
    }

    const std::vector<VkQueueFamilyProperties> queueFamiliesProperties = this->getPhysicalDeviceQueueFamilyProperties(device);

    int queueChoice = -1;
    int lastBestQueueScore = 0;
    bool supportsGraphicsAndPresentationQueue = false;

    int j=0;
    for (auto & queueFamilyProperties : queueFamiliesProperties) {
        int queueScore = 0;

        if ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            VkBool32 supportsPresentation = false;
            if (this->vulkanSurface != nullptr) vkGetPhysicalDeviceSurfaceSupportKHR(device, j, this->vulkanSurface, &supportsPresentation);
            if (supportsPresentation) supportsGraphicsAndPresentationQueue = true;

            queueScore += 10 * queueFamilyProperties.queueCount;

            if (queueScore > lastBestQueueScore) {
                lastBestQueueScore = queueScore;
                queueChoice = j;
            }
        }

        j++;
    }

    // we need a device with a graphics queue, alright
    if (!supportsGraphicsAndPresentationQueue) return std::make_tuple(0, -1);

    return std::make_tuple(score+lastBestQueueScore, queueChoice);
}

int GraphicsContext::getComputeQueueIndex(const VkPhysicalDevice & device, const bool preferSeparateQueue) {
    const std::vector<VkQueueFamilyProperties> queueFamiliesProperties = this->getPhysicalDeviceQueueFamilyProperties(device);

    const VkQueueFlags graphicsAndCompute = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

    int queueIndex = -1;

    int j=0;
    for (auto & queueFamilyProperties : queueFamiliesProperties) {
        if ((queueFamilyProperties.queueFlags & graphicsAndCompute) == graphicsAndCompute) {
            if (queueIndex == -1 || !preferSeparateQueue) {
                queueIndex = j;

                if (!preferSeparateQueue) break;
            }
        } else if ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
            if (queueIndex == -1 || preferSeparateQueue) {
                queueIndex = j;

                if (preferSeparateQueue) break;
            }
        }

        j++;
    }

    return  queueIndex;
}


bool GraphicsContext::isPhysicalDeviceSurfaceFormatsSupported(const VkPhysicalDevice & device, const VkSurfaceFormatKHR & format) {
    if (!this->isGraphicsActive()) return false;

    const std::vector<VkSurfaceFormatKHR> availableSurfaceFormats = this->queryPhysicalDeviceSurfaceFormats(device);

    for (auto & surfaceFormat : availableSurfaceFormats) {
        if (format.format == surfaceFormat.format && format.colorSpace == surfaceFormat.colorSpace) {
            return true;
        }
    }

    return false;
}

const std::vector<VkSurfaceFormatKHR> GraphicsContext::queryPhysicalDeviceSurfaceFormats(const VkPhysicalDevice & device) {
    std::vector<VkSurfaceFormatKHR> formats;

    if (!this->isVulkanActive()) return formats;

    uint32_t formatCount = 0;

    VkResult ret = vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->vulkanSurface, &formatCount, nullptr);
    if (ret != VK_SUCCESS) {
        logError("Failed to query Device Surface Formats!");
        return formats;
    }

    if (formatCount > 0) {
        formats.resize(formatCount);
        ret = vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->vulkanSurface, &formatCount, formats.data());
        
        if (ret != VK_SUCCESS) {
            logError("Failed to query Device Surface Formats!");
        }
    }

    return formats;
}

const std::vector<VkQueueFamilyProperties> GraphicsContext::getPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device) {
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    if (device == nullptr) return queueFamilyProperties;

    uint32_t deviceQueueFamilyPropertiesCount = 0;    
    vkGetPhysicalDeviceQueueFamilyProperties(device, &deviceQueueFamilyPropertiesCount, nullptr);
    if (deviceQueueFamilyPropertiesCount > 0) {
        queueFamilyProperties.resize(deviceQueueFamilyPropertiesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &deviceQueueFamilyPropertiesCount, queueFamilyProperties.data());
    }

    return queueFamilyProperties;
}

bool GraphicsContext::findDepthFormat(const VkPhysicalDevice & device, VkFormat & supportedFormat) {
    return GraphicsContext::findSupportedFormat(device,
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, supportedFormat
    );
}
    
bool GraphicsContext::findSupportedFormat(
    const VkPhysicalDevice & device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, 
    VkFormatFeatureFlags features, VkFormat & supportedFormat) {
    
    
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            supportedFormat = format;
            return true;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            supportedFormat = format;
            return true;
        }
    }

    return false;
}

bool GraphicsContext::getSurfaceCapabilities(const VkPhysicalDevice & physicalDevice, VkSurfaceCapabilitiesKHR & surfaceCapabilities) const {
    if (!this->isVulkanActive()) {
        logError("Vulkan Context not available!");
        return false;
    }

    const VkResult ret = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, this->vulkanSurface, &surfaceCapabilities);
    if (ret != VK_SUCCESS) {
        logError("Failed to get Device Surface Capabilites!");
        return false;
    }

    return true;
}


VkExtent2D GraphicsContext::getSwapChainExtent(VkSurfaceCapabilitiesKHR & surfaceCapabilities) const {
    VkExtent2D extent { 640,480 };
    if (!this->isVulkanActive()) return extent;

    int width = 0;
    int height = 0;

    SDL_Vulkan_GetDrawableSize(this->sdlWindow, &width, &height);
    
    extent.width = std::max(
        surfaceCapabilities.minImageExtent.width,
        std::min(surfaceCapabilities.maxImageExtent.width,
        static_cast<uint32_t>(width)));
    extent.height = std::max(
        surfaceCapabilities.minImageExtent.height,
        std::min(surfaceCapabilities.maxImageExtent.height,
        static_cast<uint32_t>(height)));
    
    return extent;
}

std::vector<VkPresentModeKHR> GraphicsContext::queryDeviceSwapModes(const VkPhysicalDevice & physicalDevice) const {
    uint32_t swapModesCount = 0;
    std::vector<VkPresentModeKHR> swapModes;

    if (!this->isVulkanActive()) return swapModes;

    VkResult ret = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, this->vulkanSurface, &swapModesCount, nullptr);
    if (ret == VK_SUCCESS && swapModesCount > 0) {
        swapModes.resize(swapModesCount);
        ret = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, this->vulkanSurface, &swapModesCount, swapModes.data());
    }

    return swapModes;
}

VkSurfaceKHR GraphicsContext::getVulkanSurface() const {
    return this->vulkanSurface;
}

SDL_Window * GraphicsContext::getSdlWindow() const {
    return this->sdlWindow;
}

VkInstance GraphicsContext::getVulkanInstance() const {
    return this->vulkanInstance;
}

GraphicsContext::GraphicsContext() {}

GraphicsContext::~GraphicsContext() {
    this->quitGraphics();
}

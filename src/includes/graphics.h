#ifndef SRC_INCLUDES_GRAPHICS_INCL_H_
#define SRC_INCLUDES_GRAPHICS_INCL_H_

#include "shared.h"

class GraphicsContext final {
    private:
        SDL_Window * sdlWindow = nullptr;
        VkInstance vulkanInstance = nullptr;
        VkSurfaceKHR vulkanSurface = nullptr;
        
        std::vector<const char *> vulkanExtensions;
        std::vector<const char *> vulkanLayers = {
           "VK_LAYER_KHRONOS_validation"
           //"VK_LAYER_ADRENO_debug"
        };
                
        std::vector<VkPhysicalDevice> physicalDevices;
                
        bool queryVulkanInstanceExtensions();
        const std::vector<VkExtensionProperties> queryDeviceExtensions(const VkPhysicalDevice & device) const;
        void queryPhysicalDevices();
        
        const std::vector<VkSurfaceFormatKHR> queryPhysicalDeviceSurfaceFormats(const VkPhysicalDevice & device);
        bool isPhysicalDeviceSurfaceFormatsSupported(const VkPhysicalDevice & device, const VkSurfaceFormatKHR & format);
        const std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device);
        const std::tuple<int,int> ratePhysicalDevice(const VkPhysicalDevice & device);
        
        void createVulkanInstance(const std::string & appName, const uint32_t version = VULKAN_VERSION);

        void quitWindow();
        void quitVulkan();
        void quitGraphics();

    public:
        GraphicsContext(const GraphicsContext&) = delete;
        GraphicsContext& operator=(const GraphicsContext &) = delete;
        GraphicsContext(GraphicsContext &&) = delete;
        GraphicsContext & operator=(GraphicsContext) = delete;
        
        bool isWindowActive() const;
        bool isVulkanActive() const;
        bool isGraphicsActive() const;
        
        void initWindow(const std::string & appName);
        void initVulkan(const std::string & appName, const uint32_t version);
        void initGraphics(const std::string & appName, const uint32_t version);

        const std::tuple<VkPhysicalDevice, int> pickBestPhysicalDeviceAndQueueIndex();
        bool doesPhysicalDeviceSupportExtension(const VkPhysicalDevice & device, const std::string extension) const;

        void listVulkanExtensions();
        void listLayerNames();
        void listPhysicalDevices();

        int getComputeQueueIndex(const VkPhysicalDevice & device, const bool preferSeparateQueue = false);
        
        static bool findDepthFormat(const VkPhysicalDevice & device, VkFormat & supportedFormat);
        static bool findSupportedFormat(const VkPhysicalDevice & device,
            const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat & supportedFormat);        
        
        VkSurfaceKHR getVulkanSurface() const;
        SDL_Window * getSdlWindow() const;
        VkInstance getVulkanInstance() const;
        
        VkExtent2D getSwapChainExtent(VkSurfaceCapabilitiesKHR & surfaceCapabilities) const;
        bool getSurfaceCapabilities(const VkPhysicalDevice & physicalDevice, VkSurfaceCapabilitiesKHR & surfaceCapabilities) const;
        std::vector<VkPresentModeKHR> queryDeviceSwapModes(const VkPhysicalDevice & physicalDevice) const;
                
        GraphicsContext();
        ~GraphicsContext();
};

#endif


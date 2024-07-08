#include "includes/helper.h"

std::string Helper::formatMemoryUsage(const VkDeviceSize size, const bool capAtMB) {
    if (size < KILO_BYTE) {
        return std::to_string(size) + "B";
    }

    if (size < MEGA_BYTE) {
        return std::to_string(size / KILO_BYTE) + "KB";
    }

    if (size < GIGA_BYTE || capAtMB) {
        return std::to_string(size / MEGA_BYTE) + "MB";
    }

    return std::to_string(size / GIGA_BYTE) + "GB";
}

float Helper::getRandomFloatBetween0and1() {
    return Helper::distribution(Helper::default_random_engine);
}

bool Helper::getMemoryTypeIndex(
    const VkPhysicalDevice& physicalDevice, VkMemoryRequirements& memoryRequirements,
    VkMemoryPropertyFlags preferredProperties, VkMemoryPropertyFlags alternativeProperties,
    uint32_t & memoryTypeIndex) {

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    memoryTypeIndex = ~0u;

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((memoryRequirements.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & preferredProperties) == preferredProperties) {
            memoryTypeIndex = i;
            break;;
        }
    }

    if (memoryTypeIndex != ~0u) {
        return true;
    }

    if (preferredProperties & alternativeProperties) return false;

    logError("Could not find preferred memory type for memory requirements, trying for alternatives...");

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((memoryRequirements.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & alternativeProperties) == alternativeProperties) {
            memoryTypeIndex = i;
            break;
        }
    }

    return (memoryTypeIndex != ~0u);
}

std::uniform_real_distribution<float> Helper::distribution = std::uniform_real_distribution<float>(0.0, 1.0);
std::default_random_engine Helper::default_random_engine = std::default_random_engine();


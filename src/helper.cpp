#include "includes/shared.h"
#include "includes/geometry.h"

static constexpr uint64_t KILO_BYTE = 1000;
static constexpr uint64_t MEGA_BYTE = KILO_BYTE * 1000;
static constexpr uint64_t GIGA_BYTE = MEGA_BYTE * 1000;

uint64_t Helper::getTimeInMillis() {
    const std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    return now.count();
}

std::string Helper::formatMemoryUsage(const VkDeviceSize size) {
    if (size < KILO_BYTE) {
        return std::to_string(size) + "B";
    }

    if (size < MEGA_BYTE) {
        return std::to_string(size / KILO_BYTE) + "KB";
    }

    if (size < GIGA_BYTE) {
        return std::to_string(size / MEGA_BYTE) + "MB";
    }

     return std::to_string(size / GIGA_BYTE) + "GB";
}

float Helper::getRandomFloatBetween0and1() {
    return Helper::distribution(Helper::default_random_engine);
}

void Helper::extractNormalsFromColorVertexVector(const std::vector<ColorVertex> & source, std::vector<ColorVertex> & dest, const glm::vec3 color)
{
    if (source.empty()) return;

    dest.reserve(source.size());

    for (const auto v : source) {
        dest.push_back({
            v.position,v.position, color
        });
        dest.push_back({
            v.position + v.normal, v.position + v.normal, color
        });
    }
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


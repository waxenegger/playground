#include "includes/shared.h"
#include "includes/objects.h"

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

void Helper::getNormalsFromColorVertexRenderables(const std::vector<ColorVerticesRenderable *> & source, std::vector<ColorVerticesRenderable *> & dest, const glm::vec3 color)
{
    if (source.empty()) return;

    std::vector<ColorVertex> lines;
    for (const auto & o : source) {
        for (const auto & v : o->getVertices()) {
            const glm::vec3 transformedPosition = o->getMatrix() * glm::vec4(v.position, 1.0f);
            const glm::vec3 lengthAdjustedNormal = o->getMatrix() * glm::vec4(v.position + glm::normalize(v.normal) * 0.25f, 1);

            lines.push_back( {transformedPosition,transformedPosition, color} );
            lines.push_back( {lengthAdjustedNormal, lengthAdjustedNormal, color} );
        }
    }

    auto normalsObject = new ColorVerticesRenderable(ColorVertexGeometry {lines});
    GlobalRenderableStore::INSTANCE()->registerRenderable(normalsObject);
    dest.push_back(normalsObject);
}

void Helper::getBboxesFromColorVertexRenderables(const std::vector<ColorVerticesRenderable *> & source, std::vector<ColorVerticesRenderable *> & dest, const glm::vec3 color)
{
    if (source.empty()) return;

    std::vector<ColorVertex> lines;
    for (const auto & o : source) {
        const auto & l = Helper::getBboxWireframeAsColorVertexLines(o->getBoundingBox(), color);
        lines.insert(lines.end(), l.begin(), l.end());
    }

    auto bboxesObject = new ColorVerticesRenderable(ColorVertexGeometry { lines });
    GlobalRenderableStore::INSTANCE()->registerRenderable(bboxesObject);
    dest.push_back(bboxesObject);
}

std::vector<ColorVertex> Helper::getBboxWireframeAsColorVertexLines(const BoundingBox & bbox, const glm::vec3 & color) {
    std::vector<ColorVertex> lines;

    lines.push_back({ {bbox.min.x, bbox.min.y, bbox.min.z}, {bbox.min.x, bbox.min.y, bbox.min.z}, color });
    lines.push_back({ {bbox.max.x, bbox.min.y, bbox.min.z}, {bbox.max.x, bbox.min.y, bbox.min.z}, color });

    lines.push_back({ {bbox.min.x, bbox.min.y, bbox.min.z}, {bbox.min.x, bbox.min.y, bbox.min.z}, color });
    lines.push_back({ {bbox.min.x, bbox.max.y, bbox.min.z}, {bbox.min.x, bbox.max.y, bbox.min.z}, color });

    lines.push_back({ {bbox.min.x, bbox.min.y, bbox.min.z}, {bbox.min.x, bbox.min.y, bbox.min.z}, color });
    lines.push_back({ {bbox.min.x, bbox.min.y, bbox.max.z}, {bbox.min.x, bbox.min.y, bbox.max.z}, color });

    lines.push_back({ {bbox.max.x, bbox.min.y, bbox.min.z}, {bbox.max.x, bbox.min.y, bbox.min.z}, color });
    lines.push_back({ {bbox.max.x, bbox.max.y, bbox.min.z}, {bbox.max.x, bbox.max.y, bbox.min.z}, color });

    lines.push_back({ {bbox.min.x, bbox.max.y, bbox.min.z}, {bbox.min.x, bbox.max.y, bbox.min.z}, color });
    lines.push_back({ {bbox.min.x, bbox.max.y, bbox.max.z}, {bbox.min.x, bbox.max.y, bbox.max.z}, color });

    lines.push_back({ {bbox.min.x, bbox.max.y, bbox.min.z}, {bbox.min.x, bbox.max.y, bbox.min.z}, color });
    lines.push_back({ {bbox.max.x, bbox.max.y, bbox.min.z}, {bbox.max.x, bbox.max.y, bbox.min.z}, color });

    lines.push_back({ {bbox.min.x, bbox.min.y, bbox.max.z}, {bbox.min.x, bbox.min.y, bbox.max.z}, color });
    lines.push_back({ {bbox.max.x, bbox.min.y, bbox.max.z}, {bbox.max.x, bbox.min.y, bbox.max.z}, color });

    lines.push_back({ {bbox.max.x, bbox.min.y, bbox.max.z}, {bbox.max.x, bbox.min.y, bbox.max.z}, color });
    lines.push_back({ {bbox.max.x, bbox.max.y, bbox.max.z}, {bbox.max.x, bbox.max.y, bbox.max.z}, color });

    lines.push_back({ {bbox.max.x, bbox.max.y, bbox.max.z}, {bbox.max.x, bbox.max.y, bbox.max.z}, color });
    lines.push_back({ {bbox.max.x, bbox.max.y, bbox.min.z}, {bbox.max.x, bbox.max.y, bbox.min.z}, color });

    lines.push_back({ {bbox.max.x, bbox.max.y, bbox.max.z}, {bbox.max.x, bbox.max.y, bbox.max.z}, color });
    lines.push_back({ {bbox.min.x, bbox.max.y, bbox.max.z}, {bbox.min.x, bbox.max.y, bbox.max.z}, color });

    lines.push_back({ {bbox.min.x, bbox.max.y, bbox.max.z}, {bbox.min.x, bbox.max.y, bbox.max.z}, color });
    lines.push_back({ {bbox.min.x, bbox.min.y, bbox.max.z}, {bbox.min.x, bbox.min.y, bbox.max.z}, color });

    lines.push_back({ {bbox.max.x, bbox.min.y, bbox.max.z}, {bbox.max.x, bbox.min.y, bbox.max.z}, color });
    lines.push_back({ {bbox.max.x, bbox.min.y, bbox.min.z}, {bbox.max.x, bbox.min.y, bbox.min.z}, color });

    return lines;
}

BoundingBox Helper::createBoundingBoxFromMinMax(const glm::vec3 & mins, const glm::vec3 & maxs)
{
    BoundingBox bbox;
    bbox.min = mins;
    bbox.max = maxs;

    bbox.center.x = (bbox.max.x  + bbox.min.x) / 2;
    bbox.center.y = (bbox.max.y  + bbox.min.y) / 2;
    bbox.center.z = (bbox.max.z  + bbox.min.z) / 2;

    glm::vec3 distCorner = {
        bbox.min.x - bbox.center.x,
        bbox.min.y - bbox.center.y,
        bbox.min.z - bbox.center.z
    };

    bbox.radius = glm::sqrt(distCorner.x * distCorner.x + distCorner.y * distCorner.y + distCorner.z * distCorner.z);

    return bbox;
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


#ifndef SRC_INCLUDES_HELPER_INCL_H_
#define SRC_INCLUDES_HELPER_INCL_H_

#include "models.h"

class Helper final {
    private:
        static std::default_random_engine default_random_engine;
        static std::uniform_real_distribution<float> distribution;

    public:
        Helper(const Helper&) = delete;
        Helper& operator=(const Helper &) = delete;
        Helper(Helper &&) = delete;
        Helper & operator=(Helper) = delete;

        static bool getMemoryTypeIndex(
            const VkPhysicalDevice& physicalDevice, VkMemoryRequirements& memoryRequirements,
            VkMemoryPropertyFlags preferredProperties, VkMemoryPropertyFlags alternativeProperties,
            uint32_t & memoryTypeIndex);
        static std::string formatMemoryUsage(const VkDeviceSize size, const bool capAtMB = false);

        static float getRandomFloatBetween0and1();
        static uint64_t getTimeInMillis();

        static std::vector<Vertex> getBboxWireframe(const BoundingBox & bbox);
        static BoundingBox createBoundingBoxFromMinMax(const glm::vec3 & mins = glm::vec3(0.0f), const glm::vec3 & maxs = glm::vec3(0.0f));

        static bool  checkBBoxIntersection(const BoundingBox & bbox1, const BoundingBox & bbox2);
        static BoundingBox getBoundingBox(const glm::vec3 pos, const float buffer = 0.15f);

        static std::unique_ptr<ColorMeshGeometry> createSphereColorMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec4 & color = glm::vec4(1.0f));
        static std::unique_ptr<TextureMeshGeometry> createSphereTextureMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const std::string & textureFileName);

        static std::unique_ptr<ColorMeshGeometry> createBoxColorMeshGeometry(const float & width, const float & height, const float & depth, const glm::vec4 & color = glm::vec4(1.0f));
        static std::unique_ptr<TextureMeshGeometry> createBoxTextureMeshGeometry(const float& width, const float& height, const float& depth, const std::string & textureName, const glm::vec2 & middlePoint = {1.0f/2, 2.0f/3});

        static std::unique_ptr<VertexMeshGeometry> getNormalsFromMeshRenderables(const MeshRenderableVariant & source, const glm::vec3 & color = { 1.0f, 0.0f, 0.0f });
        static std::unique_ptr<VertexMeshGeometry> getBboxesFromRenderables(const Renderable * source, const glm::vec3 & color = { 0.0f, 0.0f, 1.0f });


};

#endif


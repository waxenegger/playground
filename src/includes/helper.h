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

        static std::vector<Vertex> getBboxWireframe(const BoundingBox & bbox);

        static bool  checkBBoxIntersection(const BoundingBox & bbox1, const BoundingBox & bbox2);

        static std::unique_ptr<ColorMeshGeometry> createSphereColorMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec4 & color = glm::vec4(1.0f));
        static std::unique_ptr<TextureMeshGeometry> createSphereTextureMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const std::string & textureFileName);

        static std::unique_ptr<ColorMeshGeometry> createBoxColorMeshGeometry(const float & width, const float & height, const float & depth, const glm::vec4 & color = glm::vec4(1.0f));
        static std::unique_ptr<TextureMeshGeometry> createBoxTextureMeshGeometry(const float& width, const float& height, const float& depth, const std::string & textureName, const glm::vec2 & middlePoint = {1.0f/2, 2.0f/3});

        template<typename R>
        static std::unique_ptr<VertexMeshGeometry> getNormalsFromMeshRenderables(R * source, const glm::vec3 & color = { 1.0f, 0.0f, 0.0f })
        {
            auto lines = std::make_unique<VertexMeshGeometry>();

            VertexMesh mesh;
            mesh.color = glm::vec4(color,1);

            uint32_t idx=0;
            for (const auto & m : source->getMeshes()) {
                for (const auto & v : m.vertices) {
                    glm::vec3 transformedPosition = v.position;
                    glm::vec3 lengthAdjustedNormal = v.position + glm::normalize(v.normal) * 0.1f;

                    if constexpr(std::is_same_v<R, AnimatedModelMeshRenderable>) {
                        const auto & matrices = source->getAnimationMatrices();
                        const glm::mat4 animationMatrix = idx >= matrices.size() ? glm::mat4 { 1.0f } : matrices[idx];
                        glm::vec4 tmpVec = animationMatrix * glm::vec4(transformedPosition, 1.0f);
                        transformedPosition = tmpVec / tmpVec.w;
                        tmpVec = glm::normalize(animationMatrix * glm::vec4(lengthAdjustedNormal, 1.0f));
                        lengthAdjustedNormal = tmpVec / tmpVec.w;
                    }

                    const auto firstVertex = Vertex {transformedPosition,transformedPosition};
                    const auto secondVertex = Vertex {lengthAdjustedNormal, lengthAdjustedNormal};

                    mesh.vertices.emplace_back(firstVertex);
                    mesh.vertices.emplace_back(secondVertex);

                    idx++;
                }
            }

            lines->meshes.emplace_back(mesh);

            return lines;
        };

        // TODO: resolve later
        // make into an own request: debug request and id + type of debugging
        //static std::unique_ptr<VertexMeshGeometry> getBboxesFromRenderables(const Renderable * source, const glm::vec3 & color = { 0.0f, 0.0f, 1.0f });
};

#endif


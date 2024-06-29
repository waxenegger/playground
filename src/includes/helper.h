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
        static BoundingBox createBoundingBoxFromMinMax(const glm::vec3 & mins = glm::vec3(0.0f), const glm::vec3 & maxs = glm::vec3(0.0f))
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
        };

        static bool  checkBBoxIntersection(const BoundingBox & bbox1, const BoundingBox & bbox2);
        static BoundingBox getBoundingBox(const glm::vec3 pos, const float buffer = 0.15f);

        static std::unique_ptr<ColorMeshGeometry> createSphereColorMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const glm::vec4 & color = glm::vec4(1.0f));
        static std::unique_ptr<TextureMeshGeometry> createSphereTextureMeshGeometry(const float & radius, const uint16_t & latIntervals, const uint16_t & lonIntervals, const std::string & textureFileName);

        static std::unique_ptr<ColorMeshGeometry> createBoxColorMeshGeometry(const float & width, const float & height, const float & depth, const glm::vec4 & color = glm::vec4(1.0f));
        static std::unique_ptr<TextureMeshGeometry> createBoxTextureMeshGeometry(const float& width, const float& height, const float& depth, const std::string & textureName, const glm::vec2 & middlePoint = {1.0f/2, 2.0f/3});

        template<typename R>
        static std::unique_ptr<VertexMeshGeometry> getNormalsFromMeshRenderables(R * source, const bool useBoundingBoxWithTransforms = true, const glm::vec3 & color = { 1.0f, 0.0f, 0.0f })
        {
            auto lines = std::make_unique<VertexMeshGeometry>();

            glm::vec3 mins =  lines->bbox.min;
            glm::vec3 maxs =  lines->bbox.max;

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

                    auto vertexForBbox = glm::vec4(secondVertex.position, 1.0f);
                    if (!useBoundingBoxWithTransforms) {
                        vertexForBbox = source->getMatrix() * vertexForBbox;
                    }

                    mins.x = glm::min(mins.x, vertexForBbox.x);
                    mins.y = glm::min(mins.y, vertexForBbox.y);
                    mins.z = glm::min(mins.z, vertexForBbox.z);

                    maxs.x = glm::max(maxs.x, vertexForBbox.x);
                    maxs.y = glm::max(maxs.y, vertexForBbox.y);
                    maxs.z = glm::max(maxs.z, vertexForBbox.z);

                    mesh.vertices.emplace_back(firstVertex);
                    mesh.vertices.emplace_back(secondVertex);

                    idx++;
                }
            }

            lines->meshes.emplace_back(mesh);
            lines->bbox = Helper::createBoundingBoxFromMinMax(mins, maxs);

            return lines;
        };

        static std::unique_ptr<VertexMeshGeometry> getBboxesFromRenderables(const Renderable * source, const bool useBoundingBoxWithTransforms = true, const glm::vec3 & color = { 0.0f, 0.0f, 1.0f });


};

#endif


#ifndef SRC_INCLUDES_PIPELINES_INCL_H_
#define SRC_INCLUDES_PIPELINES_INCL_H_

#include "engine.h"

class GraphicsPipeline : public Pipeline {
    protected:
        VkPushConstantRange pushConstantRange {};
        VkSampler textureSampler = nullptr;

        Buffer vertexBuffer;
        Buffer indexBuffer;
        Buffer ssboMeshBuffer;
        Buffer ssboInstanceBuffer;
        Buffer animationMatrixBuffer;
    public:
        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline &) = delete;
        GraphicsPipeline(GraphicsPipeline &&) = delete;

        bool createGraphicsPipelineCommon(const bool doColorBlend = true, const bool hasDepth = true, const VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        virtual bool initPipeline(const PipelineConfig & config) = 0;
        virtual bool createPipeline() = 0;

        virtual void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex) = 0;
        virtual void update() = 0;

        bool isReady() const;
        bool canRender() const;

        void correctViewPortCoordinates(const VkCommandBuffer & commandBuffer);

        GraphicsPipeline(const std::string name, Renderer * renderer);
        ~GraphicsPipeline();
};

class ImGuiPipeline : public GraphicsPipeline {
    public:
        ImGuiPipeline(const std::string name, Renderer * renderer);
        ImGuiPipeline & operator=(ImGuiPipeline) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();
        bool canRender() const;

        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update();

        ~ImGuiPipeline();
};

class StaticObjectsColorVertexPipeline : public GraphicsPipeline {
    private:
        // TODO: find good memory backing model that scales

        std::vector<ColorVertex> vertices;
        std::vector<uint32_t> indexes;

        bool createBuffers();

        bool createDescriptorPool();
        bool createDescriptors();

    public:
        StaticObjectsColorVertexPipeline(const std::string name, Renderer * renderer);
        StaticObjectsColorVertexPipeline & operator=(StaticObjectsColorVertexPipeline) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();

        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update();
};


#endif

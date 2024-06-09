#ifndef SRC_INCLUDES_PIPELINE_INCL_H_
#define SRC_INCLUDES_PIPELINE_INCL_H_

#include "renderer.h"

class Shader final {
    private:
        std::string filename;
        VkShaderStageFlagBits shaderType;
        VkShaderModule shaderModule = nullptr;
        VkDevice device;

        bool readFile(const std::string & filename, std::vector<char> & buffer);
        VkShaderModule createShaderModule(const std::vector<char> & code);
    public:
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader &) = delete;
        Shader(Shader &&) = delete;
        Shader & operator=(Shader) = delete;

        VkShaderStageFlagBits getShaderType() const;
        VkShaderModule getShaderModule() const;
        std::string getFileName() const;

        Shader(const VkDevice device, const std::string & filename, const VkShaderStageFlagBits shaderType);
        ~Shader();

        bool isValid() const;
};

class Pipeline {
    protected:
        std::string name;
        std::map<std::string, const Shader *> shaders;
        bool enabled = true;
        uint32_t drawCount = 0;

        Renderer * renderer = nullptr;
        VkPipeline pipeline = nullptr;

        DescriptorPool descriptorPool;
        Descriptors descriptors;
        VkPipelineLayout layout = nullptr;

        uint8_t getNumberOfValidShaders() const;

        Pipeline * debugPipeline = nullptr;
        bool showBboxes = false;
        bool showNormals = false;

    public:
        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline &) = delete;
        Pipeline(Pipeline &&) = delete;
        Pipeline(const std::string name, Renderer * renderer);

        std::string getName() const;
        void setName(const std::string name);

        bool addShader(const std::string & file, const VkShaderStageFlagBits & shaderType);
        std::vector<VkPipelineShaderStageCreateInfo> getShaderStageCreateInfos();

        virtual bool isReady() const = 0;
        virtual bool canRender() const = 0;
        virtual bool initPipeline(const PipelineConfig & config) = 0;
        virtual bool createPipeline() = 0;
        virtual void destroyPipeline();

        bool hasPipeline() const;
        bool isEnabled();
        void setEnabled(const bool flag);

        void linkDebugPipeline(Pipeline * debugPipeline, const bool & showBboxes = true, const bool & showNormals = false);

        uint32_t getDrawCount() const;

        virtual ~Pipeline();
};

class GraphicsPipeline : public Pipeline {
    protected:
        VkPushConstantRange pushConstantRange {};
        VkSampler textureSampler = nullptr;

        int indirectBufferIndex = -1;

        Buffer vertexBuffer;
        bool usesDeviceLocalVertexBuffer = false;
        Buffer indexBuffer;
        bool usesDeviceLocalIndexBuffer = false;
        Buffer ssboMeshBuffer;
        Buffer ssboInstanceBuffer;
        Buffer animationMatrixBuffer;
    public:
        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline &) = delete;
        GraphicsPipeline(GraphicsPipeline &&) = delete;

        bool createGraphicsPipelineCommon(const bool doColorBlend = true, const bool hasDepth = true, const bool cullBack = true, const VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        virtual bool initPipeline(const PipelineConfig & config) = 0;
        virtual bool createPipeline() = 0;

        virtual void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex) = 0;
        virtual void update() = 0;

        bool isReady() const;
        bool canRender() const;

        void correctViewPortCoordinates(const VkCommandBuffer & commandBuffer);

        VkDescriptorBufferInfo getInstanceDataDescriptorInfo();

        MemoryUsage getMemoryUsage() const;
        int getIndirectBufferIndex() const;

        GraphicsPipeline(const std::string name, Renderer * renderer);
        ~GraphicsPipeline();
};

template<typename R, typename C>
class MeshPipeline : public GraphicsPipeline {
    private:
        std::vector<R *> objectsToBeRendered;
        C config;

    protected:
        bool createBuffers(const C & conf, const bool & omitIndex = false) {
            /**
            *  VERTEX BUFFER CREATION
            */

            if (conf.reservedVertexSpace == 0) {
                logError("The configuration has reserved 0 space for vertex buffers!");
                return false;
            }

            VkResult result;
            VkDeviceSize reservedSize = conf.reservedVertexSpace;

            uint64_t limit = this->usesDeviceLocalVertexBuffer ?
                this->renderer->getPhysicalDeviceProperty(ALLOCATION_LIMIT) :
                this->renderer->getPhysicalDeviceProperty(STORAGE_BUFFER_LIMIT);

            if (reservedSize > limit) {
                logError("You tried to allocate more in one go than the GPU's allocation/storage buffer limit");
                return false;
            }

            if (this->usesDeviceLocalVertexBuffer) this->renderer->trackDeviceLocalMemory(this->vertexBuffer.getSize(), true);
            this->vertexBuffer.destroy(this->renderer->getLogicalDevice());

            if (this->usesDeviceLocalVertexBuffer) {
                result = this->vertexBuffer.createDeviceLocalBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);

                if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
                    this->usesDeviceLocalVertexBuffer = false;
                } else {
                    this->renderer->trackDeviceLocalMemory(this->vertexBuffer.getSize());
                }
            }

            if (!this->usesDeviceLocalVertexBuffer) {
                result = this->vertexBuffer.createSharedStorageBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);
            }

            if (!this->vertexBuffer.isInitialized()) {
                logError("Failed to create  '" + this->name + "' Pipeline Vertex Buffer!");
                return false;
            }

            /**
            *  INDEX BUFFER CREATION
            */
            if (!omitIndex) {
                reservedSize = conf.reservedIndexSpace;
                if (reservedSize == 0) {
                    logError("Warning: The configuration has reserved 0 space for index buffers!");
                    return false;
                }

                limit = this->usesDeviceLocalIndexBuffer ?
                    this->renderer->getPhysicalDeviceProperty(ALLOCATION_LIMIT) :
                    this->renderer->getPhysicalDeviceProperty(STORAGE_BUFFER_LIMIT);

                if (reservedSize > limit) {
                    logError("You tried to allocate more in one go than the GPU's allocation/storage buffer limit");
                    return false;
                }

                if (this->usesDeviceLocalIndexBuffer) this->renderer->trackDeviceLocalMemory(this->indexBuffer.getSize(), true);
                this->indexBuffer.destroy(this->renderer->getLogicalDevice());

                if (this->usesDeviceLocalIndexBuffer) {
                    result = this->indexBuffer.createDeviceLocalBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

                    if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
                        this->usesDeviceLocalIndexBuffer = false;
                    } else {
                        this->renderer->trackDeviceLocalMemory(this->indexBuffer.getSize());
                    }
                }

                if (!this->usesDeviceLocalIndexBuffer) {
                    result = this->indexBuffer.createSharedIndexBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);
                }

                if (!this->indexBuffer.isInitialized()) {
                    logError("Failed to create  '" + this->name + "' Pipeline Index Buffer!");
                    return false;
                }
            }


            /**
            *  STORAGE BUFFER CREATION FOR INSTANCE AND MESH DATA
            *  ONLY FOR COMPUTE CULLING + INDIRECT GPU DRAW
            */

            if (USE_GPU_CULLING) {
                reservedSize = conf.reservedInstanceDataSpace;

                limit = this->renderer->getPhysicalDeviceProperty(STORAGE_BUFFER_LIMIT);
                if (reservedSize > limit) {
                    logError("You tried to allocate more in one go than the GPU's allocation/storage buffer limit");
                    return false;
                }

                this->ssboInstanceBuffer.destroy(this->renderer->getLogicalDevice());
                this->ssboInstanceBuffer.createSharedStorageBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);
                if (!this->ssboInstanceBuffer.isInitialized()) {
                    logError("Failed to create  '" + this->name + "' Pipeline SSBO Instance Buffer!");
                    return false;
                }

                reservedSize = conf.reservedMeshDataSpace;

                limit = this->renderer->getPhysicalDeviceProperty(STORAGE_BUFFER_LIMIT);
                if (reservedSize > limit) {
                    logError("You tried to allocate more in one go than the GPU's allocation/storage buffer limit");
                    return false;
                }

                this->ssboMeshBuffer.destroy(this->renderer->getLogicalDevice());
                this->ssboMeshBuffer.createSharedStorageBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), reservedSize);
                if (!this->ssboMeshBuffer.isInitialized()) {
                    logError("Failed to create  '" + this->name + "' Pipeline SSBO Mesh Data Buffer!");
                    return false;
                }
            }

            return true;
        };

        bool createDescriptorPool(const bool & withImageSampler = false) {
            if (this->renderer == nullptr || !this->renderer->isReady() || this->descriptorPool.isInitialized()) return false;

            const uint32_t count = this->renderer->getImageCount();

            this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
            this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);
            if (USE_GPU_CULLING) {
                this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);
                this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);
                this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);
            }
            if (withImageSampler) {
                this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_NUMBER_OF_TEXTURES);
            }

            this->descriptorPool.createPool(this->renderer->getLogicalDevice(), count);

            return this->descriptorPool.isInitialized();
        };

        bool createDescriptors(const bool & withImageSampler = false) {
            if (this->renderer == nullptr || !this->renderer->isReady()) return false;

            this->descriptors.destroy(this->renderer->getLogicalDevice());
            this->descriptorPool.resetPool(this->renderer->getLogicalDevice());

            this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
            this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);

            if (USE_GPU_CULLING) {
                this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
                this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
                this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
            }

            if (withImageSampler) {
                this->descriptors.addBindings(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, GlobalTextureStore::INSTANCE()->getTexures().size());
            }

            this->descriptors.create(this->renderer->getLogicalDevice(), this->descriptorPool.getPool(), this->renderer->getImageCount());

            if (!this->descriptors.isInitialized()) return false;

            const VkDescriptorBufferInfo & ssboBufferVertexInfo = this->vertexBuffer.getDescriptorInfo();

            VkDescriptorBufferInfo indirectDrawInfo;
            if (USE_GPU_CULLING) indirectDrawInfo = this->renderer->getIndirectDrawBuffer(this->indirectBufferIndex).getDescriptorInfo();
            const VkDescriptorBufferInfo & ssboInstanceBufferInfo = this->ssboInstanceBuffer.getDescriptorInfo();
            const VkDescriptorBufferInfo & ssboMeshDataBufferInfo = this->ssboMeshBuffer.getDescriptorInfo();

            std::vector<VkDescriptorImageInfo> descriptorImageInfos;
            if (withImageSampler) {
                const auto & textures = GlobalTextureStore::INSTANCE()->getTexures();
                for (auto & t : textures) {
			const VkDescriptorImageInfo & texureDescriptor = t->getDescriptorInfo();
			descriptorImageInfos.push_back(texureDescriptor);
                }
            }

            const uint32_t descSize = this->descriptors.getDescriptorSets().size();
            for (size_t i = 0; i < descSize; i++) {
                const VkDescriptorBufferInfo & uniformBufferInfo = this->renderer->getUniformBuffer(i).getDescriptorInfo();

                int j=0;
                this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, uniformBufferInfo);
                this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, ssboBufferVertexInfo);

                if (USE_GPU_CULLING) {
                    this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, indirectDrawInfo);
                    this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, ssboInstanceBufferInfo);
                    this->descriptors.updateWriteDescriptorWithBufferInfo(this->renderer->getLogicalDevice(), j++, i, ssboMeshDataBufferInfo);
                }

                if (withImageSampler) {
                    this->descriptors.updateWriteDescriptorWithImageInfo(this->renderer->getLogicalDevice(), j++, i, descriptorImageInfos);
                }
            }

            return true;
        };

        bool addObjectsToBeRenderedCommon(const void * additionalVertexData, const VkDeviceSize & additionalVertexDataSize, const std::vector<uint32_t > & additionalIndices) {
        //bool addObjectsToBeRenderedCommon(const std::vector<Vertex> & additionalVertices, const std::vector<uint32_t > & additionalIndices) {
            if (!this->vertexBuffer.isInitialized() || additionalVertexDataSize == 0) return false;

            const VkDeviceSize vertexBufferContentSize =  this->vertexBuffer.getContentSize();
            VkDeviceSize vertexBufferAdditionalContentSize =  additionalVertexDataSize;

            const VkDeviceSize indexBufferContentSize =  this->indexBuffer.getContentSize();
            VkDeviceSize indexBufferAdditionalContentSize =  additionalIndices.size() * sizeof(uint32_t);

            Buffer stagingBuffer;
            if (this->usesDeviceLocalVertexBuffer) {
                stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), vertexBufferAdditionalContentSize);
                if (stagingBuffer.isInitialized()) {
                    stagingBuffer.updateContentSize(vertexBufferAdditionalContentSize);

                    memcpy(static_cast<char *>(stagingBuffer.getBufferData()), additionalVertexData, vertexBufferAdditionalContentSize);

                    const CommandPool & pool = renderer->getGraphicsCommandPool();
                    const VkCommandBuffer & commandBuffer = pool.beginPrimaryCommandBuffer(renderer->getLogicalDevice());

                    VkBufferCopy copyRegion {};
                    copyRegion.srcOffset = 0;
                    copyRegion.dstOffset = vertexBufferContentSize;
                    copyRegion.size = vertexBufferAdditionalContentSize;
                    vkCmdCopyBuffer(commandBuffer, stagingBuffer.getBuffer(), this->vertexBuffer.getBuffer(), 1, &copyRegion);

                    pool.endCommandBuffer(commandBuffer);
                    pool.submitCommandBuffer(renderer->getLogicalDevice(), renderer->getGraphicsQueue(), commandBuffer);

                    stagingBuffer.destroy(this->renderer->getLogicalDevice());

                    this->vertexBuffer.updateContentSize(vertexBufferContentSize + vertexBufferAdditionalContentSize);
                }
            } else {
                memcpy(static_cast<char *>(this->vertexBuffer.getBufferData()) + vertexBufferContentSize, additionalVertexData, vertexBufferAdditionalContentSize);
                this->vertexBuffer.updateContentSize(vertexBufferContentSize + vertexBufferAdditionalContentSize);
            }

            if (this->indexBuffer.isInitialized() && !additionalIndices.empty()) {
                if (this->usesDeviceLocalIndexBuffer) {
                    stagingBuffer.createStagingBuffer(this->renderer->getPhysicalDevice(), this->renderer->getLogicalDevice(), indexBufferAdditionalContentSize);
                    if (stagingBuffer.isInitialized()) {
                        stagingBuffer.updateContentSize(indexBufferAdditionalContentSize);

                        memcpy(static_cast<char *>(stagingBuffer.getBufferData()), additionalIndices.data(), indexBufferAdditionalContentSize);

                        const CommandPool & pool = renderer->getGraphicsCommandPool();
                        const VkCommandBuffer & commandBuffer = pool.beginPrimaryCommandBuffer(renderer->getLogicalDevice());

                        VkBufferCopy copyRegion {};
                        copyRegion.srcOffset = 0;
                        copyRegion.dstOffset = indexBufferContentSize;
                        copyRegion.size = indexBufferAdditionalContentSize;
                        vkCmdCopyBuffer(commandBuffer, stagingBuffer.getBuffer(), this->indexBuffer.getBuffer(), 1, &copyRegion);

                        pool.endCommandBuffer(commandBuffer);
                        pool.submitCommandBuffer(renderer->getLogicalDevice(), renderer->getGraphicsQueue(), commandBuffer);

                        stagingBuffer.destroy(this->renderer->getLogicalDevice());

                        this->indexBuffer.updateContentSize(indexBufferContentSize + indexBufferAdditionalContentSize);
                    }
                } else {
                    memcpy(static_cast<char *>(this->indexBuffer.getBufferData()) + indexBufferContentSize, additionalIndices.data(), indexBufferAdditionalContentSize);
                    this->indexBuffer.updateContentSize(indexBufferContentSize + indexBufferAdditionalContentSize);
                }
            }

            return true;
        };

    public:
        MeshPipeline & operator=(MeshPipeline) = delete;
        MeshPipeline(const MeshPipeline&) = delete;
        MeshPipeline(MeshPipeline &&) = delete;
        MeshPipeline(const std::string name, Renderer * renderer) : GraphicsPipeline(name, renderer) {};

        bool initPipeline(const PipelineConfig & config);

        bool createPipeline() {
            if (!this->createDescriptors(std::is_same_v<C, TextureMeshPipelineConfig>))  {
                logError("Failed to create '" + this->name + "' Pipeline Descriptors");
                return false;
            }

            return this->createGraphicsPipelineCommon(true, true, true, this->config.topology);
        };

        bool addObjectsToBeRendered(const std::vector<R *> & objectsToBeRendered);

        void clearObjectsToBeRendered() {
            this->objectsToBeRendered.clear();
            if (this->indexBuffer.isInitialized()) this->indexBuffer.updateContentSize(0);
            if (this->vertexBuffer.isInitialized()) this->vertexBuffer.updateContentSize(0);
        };

        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update() {
            if (USE_GPU_CULLING) {
                    uint32_t i=0;
                    VkDeviceSize instanceDataSize = sizeof(ColorMeshInstanceData);

                    for (const auto & o : this->objectsToBeRendered) {
                        if (o->isDirty()) {
                            const BoundingBox & bbox = o->getBoundingBox();
                            const ColorMeshInstanceData instanceData = {
                                o->getMatrix(), bbox.center, bbox.radius
                            };

                            memcpy(static_cast<char *>(this->ssboInstanceBuffer.getBufferData()) + (i * instanceDataSize), &instanceData, instanceDataSize);
                            o->setDirty(false);
                        }
                        i++;
                    }
                }
        };

        std::vector<R *> & getRenderables() {
            return this->objectsToBeRendered;
        };

        ~MeshPipeline() {
            this->clearObjectsToBeRendered();
        };
};

using VertexMeshPipeline = MeshPipeline<VertexMeshRenderable, VertexMeshPipelineConfig>;
template<>
bool VertexMeshPipeline::initPipeline(const PipelineConfig & config);
template<>
bool VertexMeshPipeline::addObjectsToBeRendered(const std::vector<VertexMeshRenderable *> & additionalObjectsToBeRendered);
template<>
void VertexMeshPipeline::draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);

using ColorMeshPipeline = MeshPipeline<ColorMeshRenderable, ColorMeshPipelineConfig>;
template<>
bool ColorMeshPipeline::initPipeline(const PipelineConfig & config);
template<>
bool ColorMeshPipeline::addObjectsToBeRendered(const std::vector<ColorMeshRenderable *> & additionalObjectsToBeRendered);
template<>
void ColorMeshPipeline::draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);

using TextureMeshPipeline = MeshPipeline<TextureMeshRenderable, TextureMeshPipelineConfig>;
template<>
bool TextureMeshPipeline::initPipeline(const PipelineConfig & config);
template<>
bool TextureMeshPipeline::addObjectsToBeRendered(const std::vector<TextureMeshRenderable *> & additionalObjectsToBeRendered);
template<>
void TextureMeshPipeline::draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);


using MeshPipelineVariant = std::variant<std::nullptr_t, ColorMeshPipeline *, VertexMeshPipeline *, TextureMeshPipeline *>;

struct ComputePipelineConfig : PipelineConfig {
    VkDeviceSize reservedComputeSpace = 0;
    bool useDeviceLocalForComputeSpace = false;
    int indirectBufferIndex = -1;

    MeshPipelineVariant linkedGraphicsPipeline = nullptr;
};

struct CullPipelineConfig : ComputePipelineConfig {
    CullPipelineConfig(const bool indexed = true) {
        this->reservedComputeSpace = 50 * MEGA_BYTE;
        this->shaders = { { indexed ? "cull-indexed.comp.spv" : "cull-vertex.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT} };
    };
};

class ComputePipeline : public Pipeline {
    protected:
        Buffer computeBuffer;
        bool usesDeviceLocalComputeBuffer = false;
        int indirectBufferIndex = 0;
        VkPushConstantRange pushConstantRange {};

    public:
        ComputePipeline(const ComputePipeline&) = delete;
        ComputePipeline& operator=(const ComputePipeline &) = delete;
        ComputePipeline(ComputePipeline &&) = delete;
        ComputePipeline(const std::string name, Renderer * renderer);

        bool isReady() const;
        bool canRender() const;

        virtual bool initPipeline(const PipelineConfig & config) = 0;
        virtual bool createPipeline() = 0;

        virtual void update() = 0;
        virtual void compute(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex) = 0;

        MemoryUsage getMemoryUsage() const;
        int getIndirectBufferIndex() const;

        bool createComputePipelineCommon();

        ~ComputePipeline();
};

class CullPipeline : public ComputePipeline {
    private:
        uint32_t vertexOffset = 0;
        uint32_t indexOffset = 0;
        uint32_t instanceOffset = 0;
        uint32_t meshOffset = 0;

        CullPipelineConfig config;
        MeshPipelineVariant linkedGraphicsPipeline = nullptr;

        bool createDescriptorPool();
        bool createDescriptors();
        bool createComputeBuffer();

        template<typename T>
        void updateComputeBuffer(T * pipeline);
    public:
        CullPipeline(const CullPipeline&) = delete;
        CullPipeline& operator=(const CullPipeline &) = delete;
        CullPipeline(CullPipeline &&) = delete;
        CullPipeline(const std::string name, Renderer * renderer);

        void update();
        void compute(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();

        ~CullPipeline();
};

class ImGuiPipeline : public GraphicsPipeline {
    private:
        ImGUIPipelineConfig config;

    public:
        ImGuiPipeline(const std::string name, Renderer * renderer);
        ImGuiPipeline & operator=(ImGuiPipeline) = delete;
        ImGuiPipeline(const ImGuiPipeline&) = delete;
        ImGuiPipeline(ImGuiPipeline &&) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();
        bool canRender() const;

        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update();

        ~ImGuiPipeline();
};


class SkyboxPipeline : public GraphicsPipeline {
    private:
        SkyboxPipelineConfig config;

        const std::vector<glm::vec3> SKYBOX_VERTICES = {
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, 1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f),
            glm::vec3(-1.0f, 1.0f, -1.0f)
        };

        const std::vector<uint32_t> SKYBOX_INDEXES = {
            7, 4, 2, 2, 3, 7,
            5, 4, 7, 7, 6, 5,
            2, 1, 0, 0, 3, 2,
            5, 6, 0, 0, 1, 5,
            7, 3, 0, 0, 6, 7,
            4, 5, 2, 2, 5, 1
        };

        //std::array<std::string, 6> skyboxCubeImageLocations = { "front.bmp", "back.bmp", "top.bmp", "bottom.bmp", "right.bmp" , "left.bmp" };
        //std::array<std::string, 6> skyboxCubeImageLocations = { "right.png", "left.png", "top.png", "bottom.png", "front.png", "back.png" };

        Image cubeImage;
        std::vector<std::unique_ptr<Texture>> skyboxTextures;

        bool createSkybox();

        bool createDescriptorPool();
        bool createDescriptors();
    public:
        SkyboxPipeline(const std::string name, Renderer * renderer);
        SkyboxPipeline & operator=(SkyboxPipeline) = delete;
        SkyboxPipeline(const SkyboxPipeline&) = delete;
        SkyboxPipeline(SkyboxPipeline &&) = delete;

        bool initPipeline(const PipelineConfig & config);
        bool createPipeline();

        void draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex);
        void update();

        ~SkyboxPipeline();
};

#endif

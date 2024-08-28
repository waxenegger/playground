#ifndef SRC_INCLUDES_ENGINE_INCL_H_
#define SRC_INCLUDES_ENGINE_INCL_H_

#include "pipeline.h"
#include "models.h"

#include <atomic>

struct CullPipelineConfig;
class Engine final {
    private:
        static std::filesystem::path base;
        GraphicsContext * graphics = new GraphicsContext();

        Camera * camera = Camera::INSTANCE();
        Renderer * renderer = nullptr;

        std::unique_ptr<CommClient> client = nullptr;

        bool quit = false;
        uint64_t lastFrameAddedToCache = 0;
        uint32_t debugFlags = 0;

        bool addPipeline0(const std::string& name, std::unique_ptr< Pipeline >& pipe, const PipelineConfig& config, const int& index);

        template<typename P, typename C>
        bool createMeshPipeline0(const std::string & name, C & graphicsConfig, CullPipelineConfig & cullConfig);

        void createRenderer();

        void handleServerMessages(void * message);

        void inputLoopSdl();
        void render(const std::chrono::high_resolution_clock::time_point & frameStart);
    public:
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine &) = delete;
        Engine(Engine &&) = delete;
        Engine & operator=(Engine) = delete;

        bool isGraphicsActive();
        bool isReady();

        bool startNetworking(const std::string ip = "127.0.0.1", const uint16_t broadcastPort = 3000, const uint16_t requestPort = 3001);
        void send(std::shared_ptr<flatbuffers::FlatBufferBuilder> & flatbufferBuilder, std::function<void (void *)> callback = [](void * m){ if (m != nullptr) free(m);});
        void stopNetworking();

        const uint32_t getDebugFlags() const;

        template<typename P>
        P * getPipeline(const std::string name) {
            if (this->renderer == nullptr) return nullptr;

            P * ret = nullptr;

            try {
                Pipeline * p = this->renderer->getPipeline(name);
                if (p == nullptr) return nullptr;

                ret = dynamic_cast<P *>(p);
            } catch(std::bad_cast badCast) {
                logError("Failed to cast pipeline '" + name + "' to its proper type!");
                return nullptr;
            }

            return ret;
        };

        template<typename P, typename C>
        bool addPipeline(const std::string name, const C & config, const int index = -1);

        void removePipeline(const std::string name);
        void enablePipeline(const std::string name, const bool flag = true);
        void setBackDrop(const VkClearColorValue & clearColor);

        Renderer * getRenderer() const;

        bool init();
        void loop();

        bool createSkyboxPipeline();
        bool createModelPipelines(const VkDeviceSize memorySizeModels = 500 * MEGA_BYTE, const VkDeviceSize memorySizeAnimatedModels = 500 * MEGA_BYTE);
        bool createColorMeshPipelines(const VkDeviceSize memorySize = 500 * MEGA_BYTE, const VkDeviceSize memorySizeTextured = 500 * MEGA_BYTE);

        bool activateDebugging(const VkDeviceSize memorySize, const uint32_t debugFlags = 0);
        void deactivateDebugging();

        bool createColorMeshPipeline(const std::string & name, ColorMeshPipelineConfig & graphicsConfig, CullPipelineConfig & cullConfig);
        bool createVertexMeshPipeline(const std::string & name, VertexMeshPipelineConfig & graphicsConfig, CullPipelineConfig & cullConfig);
        bool createTextureMeshPipeline(const std::string & name, TextureMeshPipelineConfig & graphicsConfig, CullPipelineConfig & cullConfig);
        bool createModelMeshPipeline(const std::string & name, ModelMeshPipelineConfig & graphicsConfig, CullPipelineConfig & cullConfig);
        bool createAnimatedModelMeshPipeline(const std::string & name, AnimatedModelMeshPipelineConfig & graphicsConfig, CullPipelineConfig & cullConfig);

        bool addObjectsToBeRendered(const std::vector<ColorMeshRenderable *> & additionalObjectsToBeRendered);
        bool addObjectsToBeRendered(const std::vector<TextureMeshRenderable *> & additionalObjectsToBeRendered);
        bool addObjectsToBeRendered(const std::vector<ModelMeshRenderable *> & additionalObjectsToBeRendered);
        bool addObjectsToBeRendered(const std::vector<AnimatedModelMeshRenderable *> & additionalObjectsToBeRendered);

        bool addDebugObjectsToBeRendered(const std::vector<ColorMeshRenderable *> & additionalDebugObjectsToBeRendered);
        bool addDebugObjectsToBeRendered(const std::vector<VertexMeshRenderable *> & additionalDebugObjectsToBeRendered);
        void updateDebugObjectRenderable(VertexMeshRenderable * renderable);
        void updateDebugObjectRenderable(ColorMeshRenderable * renderable);

        bool createGuiPipeline();

        void adjustSunStrength(const float & delta = 0.1);

        void stop();

        Engine(const std::string & appName, const std::string root = "", const uint32_t version = VULKAN_VERSION);
        ~Engine();

        static std::filesystem::path getAppPath(APP_PATHS appPath);

        Camera * getCamera();
};

#endif



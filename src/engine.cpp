#include "includes/engine.h"

Engine::Engine(const std::string & appName, const std::string root, const uint32_t version) {
    logInfo("Creating Graphics Context...");
    this->graphics->initGraphics(appName, version);

    if(!this->graphics->isGraphicsActive()) {
        logError("Could not initialize Graphics Context");

        #ifdef __ANDROID__
                SDL_AndroidShowToast("Vulkan Not Supported", 1, 0, 0, 0);
                sleepInMillis(5000));
        #endif

        return;
    }

    this->graphics->listPhysicalDevices();
    logInfo("Created Vulkan Context");

    Engine::base = root;

    if (!std::filesystem::exists(Engine::base)) {
        logError("App Directory " + Engine::base.string() + "does not exist!" );
        return;
    }

    if (Engine::base.empty()) {
        const std::filesystem::path cwd = std::filesystem::current_path();
        const std::filesystem::path cwdAppPath = cwd / "assets";
        logInfo("No App Directory Supplied. Assuming '" + cwdAppPath.string() + "' ...");

        if (std::filesystem::is_directory(cwdAppPath) && std::filesystem::exists(cwdAppPath)) {
            Engine::base = cwdAppPath;
        } else {
            logError("Sub folder 'assets' does not exist!");
            return;
        }
    }

    const std::filesystem::path tempPath = Engine::getAppPath(TEMP);
    if (!std::filesystem::is_directory(tempPath)) {
        std::filesystem::remove(tempPath);
    }

    if (!std::filesystem::exists(tempPath)) {
        if (!std::filesystem::create_directory(tempPath)) {
            logError("Failed to create temporary directory!");
            return;
        }
    }

    logInfo("Base Directory: " + Engine::base.string());
}

std::filesystem::path Engine::getAppPath(APP_PATHS appPath) {
    switch(appPath) {
        case TEMP:
            return Engine::base / "temp";
        case SHADERS:
            return Engine::base / "shaders";
        case MODELS:
            return Engine::base / "models";
        case IMAGES:
            return Engine::base / "images";
        case FONTS:
            return Engine::base / "fonts";
        case MAPS:
            return Engine::base / "maps";
        case ROOT:
        default:
            return Engine::base;
    }
}

bool Engine::startNetworking(const std::string ip, const uint16_t udpPort, const uint16_t tcpPort)
{
    if (ip == "127.0.0.1" || ip == "localhost") {
        // use ipc for local to mimic traffic
        this->server = std::make_unique<CommServer>();
        if (this->server == nullptr) return false;
        if (!this->server->start()) return false;

        this->client = std::make_unique<CommClient>(this->server->getInProcContext());
        if (this->client == nullptr) return false;
        if (!this->client->start()) return false;

        Communication::sleepInMillis(1000);
    } else {
        // connect to remote server
        if (this->client != nullptr) this->client->stop();

        this->client = std::make_unique<CommClient>(ip, udpPort, tcpPort);
        if (this->client == nullptr) return false;

        if (!this->client->start()) return false;
    }

    this->client->sendAsync("hjojosdf isdf s ssgjd gdfgndf", true, [](const std::string r) { logInfo("HAHA " + r); });

    return true;
}

void Engine::stopNetworking()
{
    if (this->client != nullptr) {
        this->client->stop();
        this->client = nullptr;
    }
    if (this->server != nullptr) {
        this->server->stop();
        this->server = nullptr;
    }
}

bool Engine::isGraphicsActive() {
    return this->graphics != nullptr && this->graphics->isGraphicsActive();
}

bool Engine::isReady() {
    return this->graphics->isGraphicsActive() && this->renderer != nullptr && this->renderer->canRender();
}

void Engine::loop() {
    if (!this->isReady()) return;

    this->renderer->resume();

    logInfo("Starting Render Loop...");

    this->inputLoopSdl();

    logInfo("Ended Render Loop");
}

bool Engine::init() {
    if(!this->graphics->isGraphicsActive()) return false;

    this->createRenderer();
    if (this->renderer == nullptr) return false;

    if (!renderer->initRenderer()) return false;

    VkExtent2D windowSize = this->renderer->getSwapChainExtent();
    this->camera->setAspectRatio(static_cast<float>(windowSize.width) / windowSize.height);

    return true;
}

Renderer * Engine::getRenderer() const
{
    return this->renderer;
}

void Engine::createRenderer() {
    logInfo("Creating Renderer...");

    const std::tuple<VkPhysicalDevice, int> bestRenderDevice = this->graphics->pickBestPhysicalDeviceAndQueueIndex();
    const VkPhysicalDevice defaultPhysicalDevice = std::get<0>(bestRenderDevice);
    if (defaultPhysicalDevice == nullptr) {
        logError("Failed to find suitable physical Device!");
        return;
    }

    VkPhysicalDeviceProperties physicalProperties;
    vkGetPhysicalDeviceProperties(defaultPhysicalDevice, &physicalProperties);
    const bool isDedicatedCard = physicalProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    logInfo("Best Device:\t" + std::string(physicalProperties.deviceName));
    logInfo("Is Dedicated GPU:\t" + std::string(isDedicatedCard ? "TRUE" : "FALSE"));

    const int defaultQueueIndex = std::get<1>(bestRenderDevice);
    // for now prefer both compute and graphics on same queue
    const int defaultComputeIndex = this->graphics->getComputeQueueIndex(defaultPhysicalDevice, false);
    // we could have no combined compute/graphics queue
    if (defaultComputeIndex < 0) {
        USE_GPU_CULLING = false;
        logError("Your hardware has no combined compute/graphics queue. Falling back onto CPU frustum culling");
    }

    renderer = new Renderer(this->graphics, defaultPhysicalDevice, defaultQueueIndex, defaultComputeIndex);

    if (!renderer->isReady()) {
        logError("Failed to initialize Renderer!");
    }

    logInfo("Renderer is Ready");
}

void Engine::render(const std::chrono::high_resolution_clock::time_point & frameStart) {
    Camera::INSTANCE()->update(this->renderer->getDeltaTime());
    Camera::INSTANCE()->updateFrustum();

    this->renderer->render();

    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_span = now - frameStart;

    this->renderer->addDeltaTime(now, time_span.count());
}

void Engine::inputLoopSdl() {
    SDL_Event e;
    bool isFullScreen = false;
    bool needsRestoreAfterFullScreen = false;

    SDL_StartTextInput();

    while(!this->quit) {
        const std::chrono::high_resolution_clock::time_point frameStart = std::chrono::high_resolution_clock::now();

        while (SDL_PollEvent(&e) != 0) {
            switch(e.type) {
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED ||
                        e.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                        e.window.event == SDL_WINDOWEVENT_MINIMIZED ||
                        e.window.event == SDL_WINDOWEVENT_RESTORED) {
                            if (this->renderer != nullptr && !this->renderer->isPaused()) {
                                this->renderer->forceRenderUpdate(true);
                                if (isFullScreen && this->renderer->isMaximized()) {
                                    SDL_SetWindowFullscreen(this->graphics->getSdlWindow(), SDL_WINDOW_FULLSCREEN);
                                }
                            }
                    }
                    break;
                case SDL_KEYDOWN:
                    switch (e.key.keysym.scancode) {
                        case SDL_SCANCODE_1:
                        {
                            this->setBackDrop(BLACK);
                            break;
                        }
                        case SDL_SCANCODE_2:
                        {
                            this->setBackDrop(WHITE);
                            break;
                        }
                        case SDL_SCANCODE_3:
                        {
                            //TODO: remove, for testing only

                            const auto & nrOfRenderables = GlobalRenderableStore::INSTANCE()->getNumberOfRenderables();
                            auto k = GlobalRenderableStore::INSTANCE()->getRenderablesByIndex<ModelMeshRenderable>(nrOfRenderables-1);
                            if (k != nullptr) k->setPosition({0,30,0});

                            std::vector<TextureMeshRenderable *> renderables;

                            GlobalTextureStore::INSTANCE()->uploadTexture("dice", "dice.png", this->getRenderer(), true);

                            const auto & boxGeom = Helper::createBoxTextureMeshGeometry(15, 15, 15, "dice");
                            auto boxMeshRenderable = std::make_unique<TextureMeshRenderable>("dice", boxGeom);
                            auto boxRenderable = GlobalRenderableStore::INSTANCE()->registerRenderable<TextureMeshRenderable>(boxMeshRenderable);
                            renderables.emplace_back(boxRenderable);

                            boxRenderable->setPosition(glm::vec3(20,20,20));
                            this->addObjectsToBeRendered(renderables);

                            break;
                        }
                        case SDL_SCANCODE_P:
                        {
                            if (this->server != nullptr) this->server->send("GFSgdgdgdr       hrtjihjrthjth dsdfdg fdgfhdfhggf ghfghgfh");

                            // TODO: remove, for testing
                            auto bob = GlobalRenderableStore::INSTANCE()->getRenderablesByName<AnimatedModelMeshRenderable>("bob");
                            if (bob != nullptr) {
                                bob->changeCurrentAnimationTime(1.0f);
                            }

                            auto stego = GlobalRenderableStore::INSTANCE()->getRenderablesByName<AnimatedModelMeshRenderable>("stego");
                            if (stego != nullptr) stego->changeCurrentAnimationTime(25.0f);

                            auto stego2 = GlobalRenderableStore::INSTANCE()->getRenderablesByName<AnimatedModelMeshRenderable>("stego2");
                            if (stego2 != nullptr) stego2->changeCurrentAnimationTime(50.0f);

                            auto cesium = GlobalRenderableStore::INSTANCE()->getRenderablesByName<AnimatedModelMeshRenderable>("cesium");
                            if (cesium != nullptr) {
                                const Direction d{false, true, false, false};
                                cesium->changeCurrentAnimationTime(10.0f);
                                auto p = cesium->getPosition();
                                p.x++;
                                cesium->setPosition(p);
                            }

                            auto dice = GlobalRenderableStore::INSTANCE()->getRenderablesByName<TextureMeshRenderable>("dice");
                            if (dice != nullptr) {
                                const Direction d{false, true, false, false};
                                auto p = dice->getPosition();
                                p.y++;
                                dice->setPosition(p);
                                dice->rotate(45, 45,0);
                            }

                            break;
                        }
                        case SDL_SCANCODE_KP_PLUS:
                        {
                            this->adjustSunStrength(+0.1f);
                            break;
                        }
                        case SDL_SCANCODE_KP_MINUS:
                        {
                            this->adjustSunStrength(-0.1f);
                            break;

                        }
                        case SDL_SCANCODE_W:
                        {
                            this->camera->move(Camera::KeyPress::UP, true);
                            break;
                        }
                        case SDL_SCANCODE_S:
                        {
                            this->camera->move(Camera::KeyPress::DOWN, true);
                            break;
                        }
                        case SDL_SCANCODE_A:
                        {
                            this->camera->move(Camera::KeyPress::LEFT, true);
                            break;
                        }
                        case SDL_SCANCODE_D:
                        {
                            this->camera->move(Camera::KeyPress::RIGHT, true);
                            break;
                        }
                        case SDL_SCANCODE_F:
                        {
                            this->renderer->setShowWireFrame(!this->renderer->doesShowWireFrame());
                            break;
                        }
                        case SDL_SCANCODE_F12:
                        {
                            if (this->renderer != nullptr && !this->renderer->isPaused()) {
                                isFullScreen = !this->renderer->isFullScreen();
                                if (isFullScreen) {
                                    if (this->renderer->isMaximized()) {
                                        SDL_SetWindowFullscreen(this->graphics->getSdlWindow(), SDL_WINDOW_FULLSCREEN);
                                    } else {
                                        needsRestoreAfterFullScreen = true;
                                        SDL_MaximizeWindow(this->graphics->getSdlWindow());
                                    }
                                } else {
                                    SDL_SetWindowFullscreen(this->graphics->getSdlWindow(), SDL_FALSE);
                                    if (needsRestoreAfterFullScreen) {
                                        SDL_RestoreWindow(this->graphics->getSdlWindow());
                                        needsRestoreAfterFullScreen = false;
                                    }
                                }
                            }
                            break;
                        }
                        case SDL_SCANCODE_F5:
                        {
                            // TODO: remove, for testing only
                            if (Camera::INSTANCE()->isInThirdPersonMode()) {
                                Camera::INSTANCE()->linkToRenderable(nullptr);
                            } else {
                                auto stego = GlobalRenderableStore::INSTANCE()->getRenderablesByName<Renderable>("stego");
                                if (stego != nullptr) Camera::INSTANCE()->linkToRenderable(stego);
                            }
                            break;
                        }
                        case SDL_SCANCODE_Q:
                            quit = true;
                            break;
                        default:
                            break;
                    };
                    break;
                case SDL_KEYUP:
                    switch (e.key.keysym.scancode) {
                        case SDL_SCANCODE_W:
                        {
                            this->camera->move(Camera::KeyPress::UP);
                            break;
                        }
                        case SDL_SCANCODE_S:
                        {
                            this->camera->move(Camera::KeyPress::DOWN);
                            break;
                        }
                        case SDL_SCANCODE_A:
                        {
                            this->camera->move(Camera::KeyPress::LEFT);
                            break;
                        }
                        case SDL_SCANCODE_D:
                        {
                            this->camera->move(Camera::KeyPress::RIGHT);
                            break;
                        }
                        default:
                            break;
                    };
                    break;
                case SDL_MOUSEMOTION:
                {
                    if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
                        this->camera->accumulateRotationDeltas(static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel));
                    }
                    break;
                }
                case SDL_MOUSEWHEEL:
                {
                    const Sint32 delta = e.wheel.y * (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL ? 1 : -1);
                    float newFovy = this->camera->getFovY() - delta * 2;
                    if (newFovy < 1) newFovy = 1;
                    else if (newFovy > 45) newFovy = 45;
                    this->camera->setFovY(newFovy);
                    break;
                }
                case SDL_MOUSEBUTTONUP:
                {
                    if (e.button.button == SDL_BUTTON_RIGHT) {
                        SDL_SetRelativeMouseMode(SDL_GetRelativeMouseMode() == SDL_TRUE ? SDL_FALSE : SDL_TRUE);
                    }
                    break;
                }
                case SDL_QUIT:
                    quit = true;
                    break;
            }

            const auto guiPipeline = this->getPipeline<Pipeline>(GUI_PIPELINE);
            if (guiPipeline != nullptr && guiPipeline->isEnabled() && SDL_GetRelativeMouseMode() == SDL_FALSE) {
                ImGui_ImplSDL2_ProcessEvent(&e);
            }
        }

        this->render(frameStart);
    }

    this->stopNetworking();

    SDL_StopTextInput();
}

void Engine::setBackDrop(const VkClearColorValue & clearColor) {
    if (this->renderer == nullptr) return;

    this->renderer->setClearValue(clearColor);
}

void Engine::removePipeline(const std::string name)
{
    if (this->renderer == nullptr) return;

    this->renderer->removePipeline(name);
}

void Engine::enablePipeline(const std::string name, const bool flag)
{
    if (this->renderer == nullptr) return;

    this->renderer->enablePipeline(name, flag);
}

Camera * Engine::getCamera() {
    return this->camera;
}

template<typename P, typename C>
bool Engine::addPipeline(const std::string name, const C & config, const int index)
{
    static_assert("Only Pipeline Types are allowed!");
    return false;
};

template<>
bool Engine::addPipeline<VertexMeshPipeline>(const std::string name, const VertexMeshPipelineConfig & config, const int index)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<VertexMeshPipeline>(name, this->renderer);
    return this->addPipeline0(name, pipe, config, index);
}

template<>
bool Engine::addPipeline<ColorMeshPipeline>(const std::string name, const ColorMeshPipelineConfig & config, const int index)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<ColorMeshPipeline>(name, this->renderer);
    return this->addPipeline0(name, pipe, config, index);
}

template<>
bool Engine::addPipeline<TextureMeshPipeline>(const std::string name, const TextureMeshPipelineConfig & config, const int index)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<TextureMeshPipeline>(name, this->renderer);
    return this->addPipeline0(name, pipe, config, index);
}

template<>
bool Engine::addPipeline<ModelMeshPipeline>(const std::string name, const ModelMeshPipelineConfig & config, const int index)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<ModelMeshPipeline>(name, this->renderer);
    return this->addPipeline0(name, pipe, config, index);
}

template<>
bool Engine::addPipeline<AnimatedModelMeshPipeline>(const std::string name, const AnimatedModelMeshPipelineConfig & config, const int index)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<AnimatedModelMeshPipeline>(name, this->renderer);
    return this->addPipeline0(name, pipe, config, index);
}

template<>
bool Engine::addPipeline<SkyboxPipeline>(const std::string name, const SkyboxPipelineConfig & config, const int index)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<SkyboxPipeline>(name, this->renderer);
    return this->addPipeline0(name, pipe, config, index);
}

template<>
bool Engine::addPipeline<ImGuiPipeline>(const std::string name, const ImGUIPipelineConfig & config, const int index)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<ImGuiPipeline>(name, this->renderer);
    return this->addPipeline0(name, pipe, config, index);
}

template<>
bool Engine::addPipeline<CullPipeline>(const std::string name, const CullPipelineConfig & config, const int index)
{
    std::unique_ptr<Pipeline> pipe = std::make_unique<CullPipeline>(name, this->renderer);
    return this->addPipeline0(name, pipe, config, index);
}

bool Engine::addPipeline0(const std::string & name, std::unique_ptr<Pipeline> & pipe, const PipelineConfig & config, const int & index) {
    if (!pipe->initPipeline(config)) {
        logError("Failed to init Pipeline: " + name);
        return false;
    }

    return this->renderer->addPipeline(pipe, index);
}

bool Engine::createSkyboxPipeline() {
    const SkyboxPipelineConfig & config = {};
    return this->addPipeline<SkyboxPipeline>(SKYBOX_PIPELINE, config);
}

bool Engine::createModelPipelines(const VkDeviceSize memorySizeModels, const VkDeviceSize memorySizeAnimatedModels, const bool displayNormals, const bool displayBboxes) {
    bool ret = true;

    ModelMeshPipelineConfig modelConf {};
    modelConf.reservedVertexSpace = memorySizeModels;
    modelConf.reservedIndexSpace = memorySizeModels;

    CullPipelineConfig cullConfModels {};
    if (this->createModelMeshPipeline(MODELS_PIPELINE, modelConf, cullConfModels)) {
        if (displayNormals || displayBboxes) this->createDebugPipeline(MODELS_PIPELINE, displayBboxes, displayNormals);
    } else ret = false;

    AnimatedModelMeshPipelineConfig animatedModelConf {};
    animatedModelConf.reservedVertexSpace = memorySizeAnimatedModels;
    animatedModelConf.reservedIndexSpace = memorySizeAnimatedModels;

    CullPipelineConfig cullConfAnimatedModels {};
    if (this->createAnimatedModelMeshPipeline(ANIMATED_MODELS_PIPELINE, animatedModelConf, cullConfAnimatedModels)) {
        if (displayNormals || displayBboxes) this->createDebugPipeline(ANIMATED_MODELS_PIPELINE, displayBboxes, displayNormals);
    } else ret = false;

    return ret;
}

bool Engine::createColorMeshPipelines(const VkDeviceSize memorySize, const VkDeviceSize memorySizeTextured, const bool displayNormals, const bool displayBboxes) {
    bool ret = true;

    ColorMeshPipelineConfig colorMeshConf {};
    colorMeshConf.useDeviceLocalForVertexSpace = true;
    colorMeshConf.useDeviceLocalForIndexSpace = true;
    colorMeshConf.reservedVertexSpace = memorySize;
    colorMeshConf.reservedIndexSpace = memorySize;

    CullPipelineConfig colorMeshCullConf {};
    if (this->createColorMeshPipeline(COLOR_MESH_PIPELINE, colorMeshConf, colorMeshCullConf)) {
        if (displayNormals || displayBboxes) this->createDebugPipeline(COLOR_MESH_PIPELINE, displayBboxes, displayNormals);
    } else ret = false;

    TextureMeshPipelineConfig textureMeshConf {};
    textureMeshConf.useDeviceLocalForVertexSpace = true;
    textureMeshConf.useDeviceLocalForIndexSpace = true;
    textureMeshConf.reservedVertexSpace = memorySizeTextured;
    textureMeshConf.reservedIndexSpace = memorySizeTextured;

    CullPipelineConfig textureMeshCullConf {};
    if (this->createTextureMeshPipeline(TEXTURE_MESH_PIPELINE, textureMeshConf, textureMeshCullConf)) {
        if (displayNormals || displayBboxes) this->createDebugPipeline(TEXTURE_MESH_PIPELINE, displayBboxes, displayNormals);
    } else ret = false;

    return ret;
}

bool Engine::createVertexMeshPipeline(const std::string & name, VertexMeshPipelineConfig & graphicsConfig, CullPipelineConfig & cullConfig) {
    return this->createMeshPipeline0<VertexMeshPipeline, VertexMeshPipelineConfig>(name, graphicsConfig, cullConfig);
}

bool Engine::createColorMeshPipeline(const std::string & name, ColorMeshPipelineConfig & graphicsConfig, CullPipelineConfig & cullConfig) {
    return this->createMeshPipeline0<ColorMeshPipeline, ColorMeshPipelineConfig>(name, graphicsConfig, cullConfig);
}

bool Engine::createTextureMeshPipeline(const std::string & name, TextureMeshPipelineConfig & graphicsConfig, CullPipelineConfig & cullConfig) {
    return this->createMeshPipeline0<TextureMeshPipeline, TextureMeshPipelineConfig>(name, graphicsConfig, cullConfig);
}

bool Engine::createModelMeshPipeline(const std::string & name, ModelMeshPipelineConfig & graphicsConfig, CullPipelineConfig & cullConfig) {
    return this->createMeshPipeline0<ModelMeshPipeline, ModelMeshPipelineConfig>(name, graphicsConfig, cullConfig);
}

bool Engine::createAnimatedModelMeshPipeline(const std::string & name, AnimatedModelMeshPipelineConfig & graphicsConfig, CullPipelineConfig & cullConfig) {
    return this->createMeshPipeline0<AnimatedModelMeshPipeline, AnimatedModelMeshPipelineConfig>(name, graphicsConfig, cullConfig);
}

bool Engine::addObjectsToBeRendered(const std::vector<ColorMeshRenderable *>& additionalObjectsToBeRendered)
{
    auto pipe = this->getPipeline<ColorMeshPipeline>(COLOR_MESH_PIPELINE);
    if (pipe == nullptr) {
        logError("Engine lacks a suitable pipeline to render the objects!");
        return false;
    }

    return pipe->addObjectsToBeRendered(additionalObjectsToBeRendered, true);
}

bool Engine::addObjectsToBeRendered(const std::vector<TextureMeshRenderable *>& additionalObjectsToBeRendered)
{
    auto pipe = this->getPipeline<TextureMeshPipeline>(TEXTURE_MESH_PIPELINE);
    if (pipe == nullptr) {
        logError("Engine lacks a suitable pipeline to render the objects!");
        return false;
    }

    return pipe->addObjectsToBeRendered(additionalObjectsToBeRendered, true);
}

bool Engine::addObjectsToBeRendered(const std::vector<ModelMeshRenderable *>& additionalObjectsToBeRendered)
{
    auto pipe = this->getPipeline<ModelMeshPipeline>(MODELS_PIPELINE);
    if (pipe == nullptr) {
        logError("Engine lacks a suitable pipeline to render the objects!");
        return false;
    }

    return pipe->addObjectsToBeRendered(additionalObjectsToBeRendered, true);
}

bool Engine::addObjectsToBeRendered(const std::vector<AnimatedModelMeshRenderable *>& additionalObjectsToBeRendered)
{
    auto pipe = this->getPipeline<AnimatedModelMeshPipeline>(ANIMATED_MODELS_PIPELINE);
    if (pipe == nullptr) {
        logError("Engine lacks a suitable pipeline to render the objects!");
        return false;
    }

    return pipe->addObjectsToBeRendered(additionalObjectsToBeRendered, true);
}

template <typename P, typename C>
bool Engine::createMeshPipeline0(const std::string & name, C & graphicsConfig, CullPipelineConfig & cullConfig) {
    const int optionalIndirectBufferIndex = this->renderer->getNextIndirectBufferIndex();
    if (USE_GPU_CULLING && optionalIndirectBufferIndex < 0) {
        logError("Could not create GPU culled Graphics Pipeline because there are no more free indirect buffers. Increase the limit.");
        return false;
    }

    graphicsConfig.indirectBufferIndex = optionalIndirectBufferIndex;
    if (!this->addPipeline<P>(name, graphicsConfig)) {
        this->removePipeline(name + "-cull");
        return false;
    }

    if (USE_GPU_CULLING) {
        cullConfig.indirectBufferIndex = optionalIndirectBufferIndex;

        auto graphicsPipelineToBeLinked = this->getPipeline<P>(name);
        if (graphicsPipelineToBeLinked == nullptr) return false;

        cullConfig.linkedGraphicsPipeline = MeshPipelineVariant { graphicsPipelineToBeLinked };

        if (!this->addPipeline<CullPipeline>(name + "-cull", cullConfig)) return false;
    }

    return true;
}

bool Engine::createDebugPipeline(const std::string & pipelineToDebugName, const bool & showBboxes, const bool & showNormals)
{
    auto pipelineToDebug = this->getPipeline<Pipeline>(pipelineToDebugName);
    if (pipelineToDebug == nullptr) {
        logError("Please create the pipeline that you want to show debug geometry first!");
        return false;
    }

    VertexMeshPipelineConfig graphicsConfig;
    graphicsConfig.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    graphicsConfig.reservedVertexSpace = 500 * MEGA_BYTE;

    // set the info that's needed to link to the pipeline we want to debug'
    graphicsConfig.pipelineToDebug = pipelineToDebug;
    graphicsConfig.showNormals = showNormals;
    graphicsConfig.showBboxes = showBboxes;

    CullPipelineConfig cullConfig(false);

    return this->createMeshPipeline0<VertexMeshPipeline, VertexMeshPipelineConfig>(pipelineToDebugName + "-debug", graphicsConfig, cullConfig);
}

bool Engine::createGuiPipeline()
{
    const ImGUIPipelineConfig & config {};
    return this->addPipeline<ImGuiPipeline>(GUI_PIPELINE, config);
}

void Engine::adjustSunStrength(const float & delta)
{
    const float presentStrength = Renderer::SUN_LOCATION_STRENGTH.w;
    float newStrength = glm::clamp<float>(presentStrength+delta, 0.0f, 1.0f);
    Renderer::SUN_LOCATION_STRENGTH.w = newStrength;
}

void Engine::stop()
{
    this->stopNetworking();
    this->quit = true;
}

Engine::~Engine() {
    if (this->renderer != nullptr) {
        delete this->renderer;
        this->renderer = nullptr;
    }

    if (this->graphics != nullptr) {
        delete this->graphics;
        this->graphics = nullptr;
    }

    Camera::INSTANCE()->destroy();
}

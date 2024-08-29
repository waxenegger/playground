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
    return ::getAppPath(Engine::base, appPath);
}

void Engine::handleServerMessages(void * message)
{
    if (message == nullptr || this->quit) return;

    auto deleter = [](void * data ) {
        if (data != nullptr) free(data);
    };

    std::unique_ptr<void, decltype(deleter)> wrappedMessage(message, deleter);

    const auto m = GetMessage(static_cast<uint8_t *>(wrappedMessage.get()));
    if (m == nullptr) return;

    //const auto messageDebugFlags = m->debug();

    const auto contentVector = m->content();
    if (contentVector == nullptr) return;

    const auto contentVectorType = m->content_type();
    if (contentVectorType == nullptr) return;

    const auto getBoundingSphere = [](const UpdatedObjectProperties * props) -> BoundingSphere {
        BoundingSphere boundingSphere;
        if (props == nullptr) return boundingSphere;

        boundingSphere.radius = props->sphere_radius();
        boundingSphere.center = glm::vec3(
            props->sphere_center()->x(),
            props->sphere_center()->y(),
            props->sphere_center()->z()
        );

        return boundingSphere;
    };

    const uint32_t nrOfMessages = contentVector->size();
    for (uint32_t i=0;i<nrOfMessages;i++) {
        if (this->quit) break;

        const auto messageType = (const MessageUnion) (*contentVectorType)[i];

        switch(messageType) {
            case MessageUnion_ObjectCreateAndUpdateRequest:
            {
                const auto request = (const ObjectCreateAndUpdateRequest *)  (*contentVector)[i];
                switch(request->object_type()) {
                    case ObjectUpdateRequestUnion_SphereUpdateRequest:
                    {
                        const auto sphere = request->object_as_SphereUpdateRequest();
                        const auto id = sphere->updates()->id()->str();
                        const auto texture = sphere->texture()->str();
                        const auto radius = sphere->radius();
                        const auto matrix = sphere->updates()->matrix();
                        const auto rot = sphere->updates()->rotation();

                        if (!texture.empty()) {
                            const auto & t = GlobalTextureStore::INSTANCE()->uploadTexture(texture, this->renderer, true);
                            auto sphereGeom = Helper::createSphereTextureMeshGeometry(sphere->radius(), 20, 20, t);
                            if (sphereGeom == nullptr) continue;
                            sphereGeom->sphere = getBoundingSphere(sphere->updates());
                            auto sphereMeshRenderable = std::make_unique<TextureMeshRenderable>(id, sphereGeom);
                            auto sphereRenderable = GlobalRenderableStore::INSTANCE()->registerObject<TextureMeshRenderable>(sphereMeshRenderable);
                            sphereRenderable->setMatrix(matrix);
                            sphereRenderable->setRotation({rot->x(), rot->y(), rot->z()});
                            sphereRenderable->setScaling(sphere->updates()->scaling());
                            this->addObjectsToBeRendered({ sphereRenderable});
                        } else {
                            const auto color = sphere->color();
                            auto sphereGeom = Helper::createSphereColorMeshGeometry(radius, 20, 20, glm::vec4(color->x(), color->y(), color->z(), color->w()));
                            sphereGeom->sphere = getBoundingSphere(sphere->updates());
                            auto sphereMeshRenderable = std::make_unique<ColorMeshRenderable>(id, sphereGeom);
                            auto sphereRenderable = GlobalRenderableStore::INSTANCE()->registerObject<ColorMeshRenderable>(sphereMeshRenderable);
                            sphereRenderable->setMatrix(matrix);
                            sphereRenderable->setRotation({rot->x(), rot->y(), rot->z()});
                            sphereRenderable->setScaling(sphere->updates()->scaling());
                            this->addObjectsToBeRendered({ sphereRenderable});
                        }
                        break;
                    }
                    case ObjectUpdateRequestUnion_BoxUpdateRequest:
                    {
                        const auto box = request->object_as_BoxUpdateRequest();
                        const auto id = box->updates()->id()->str();
                        const auto texture = box->texture()->str();
                        const auto width = box->width();
                        const auto height = box->height();
                        const auto depth = box->depth();
                        const auto matrix = box->updates()->matrix();
                        const auto rot = box->updates()->rotation();

                        if (!texture.empty()) {
                            const auto & t = GlobalTextureStore::INSTANCE()->uploadTexture(texture, this->renderer, true);
                            auto boxGeom = Helper::createBoxTextureMeshGeometry(width, height, depth, t);
                            boxGeom->sphere = getBoundingSphere(box->updates());
                            auto boxMeshRenderable = std::make_unique<TextureMeshRenderable>(id, boxGeom);
                            auto boxRenderable = GlobalRenderableStore::INSTANCE()->registerObject<TextureMeshRenderable>(boxMeshRenderable);
                            boxRenderable->setMatrix(matrix);
                            boxRenderable->setRotation({rot->x(), rot->y(), rot->z()});
                            boxRenderable->setScaling(box->updates()->scaling());
                            this->addObjectsToBeRendered({ boxRenderable});
                        } else {
                            const auto color = box->color();
                            auto boxGeom = Helper::createBoxColorMeshGeometry(width, height, depth, { color->x(), color->y(), color->z(), color->w()});
                            boxGeom->sphere = getBoundingSphere(box->updates());
                            auto boxMeshRenderable = std::make_unique<ColorMeshRenderable>(id, boxGeom);
                            auto boxRenderable = GlobalRenderableStore::INSTANCE()->registerObject<ColorMeshRenderable>(boxMeshRenderable);
                            boxRenderable->setMatrix(matrix);
                            boxRenderable->setRotation({rot->x(), rot->y(), rot->z()});
                            boxRenderable->setScaling(box->updates()->scaling());
                            this->addObjectsToBeRendered({ boxRenderable});
                        }

                        break;
                    }
                    case ObjectUpdateRequestUnion_ModelUpdateRequest:
                    {
                        const auto model = request->object_as_ModelUpdateRequest();
                        const auto id = model->updates()->id()->str();
                        const auto file = model->file()->str();
                        const auto matrix = model->updates()->matrix();
                        const auto animation = model->animation()->str();
                        const auto animationTime = model->animation_time();
                        const auto rot = model->updates()->rotation();

                        const auto flags = model->flags();
                        const auto useFirstChildAsRoot = model->first_child_root();

                        const auto m = Model::loadFromAssetsFolder(id, file, flags, useFirstChildAsRoot);
                        if (m.has_value()) {
                            if (animation.empty()) {
                                auto modelRenderable = std::get<ModelMeshRenderable *>(m.value());
                                modelRenderable->setMatrix(matrix);
                                modelRenderable->setRotation({rot->x(), rot->y(), rot->z()});
                                modelRenderable->setScaling(model->updates()->scaling());
                                modelRenderable->setBoundingSphere(getBoundingSphere(model->updates()));
                                this->addObjectsToBeRendered({ modelRenderable});
                            } else {
                                auto modelRenderable = std::get<AnimatedModelMeshRenderable *>(m.value());
                                modelRenderable->setMatrix(matrix);
                                modelRenderable->setRotation({rot->x(), rot->y(), rot->z()});
                                modelRenderable->setScaling(model->updates()->scaling());
                                modelRenderable->setBoundingSphere(getBoundingSphere(model->updates()));
                                modelRenderable->setCurrentAnimation(animation);
                                modelRenderable->setCurrentAnimationTime(animationTime);
                                this->addObjectsToBeRendered({ modelRenderable});
                            }

                            this->renderer->forceNewTexturesUpload();
                        }
                        break;
                    }
                    case ObjectUpdateRequestUnion_NONE:
                        break;
                }
                break;
            }
            case MessageUnion_ObjectUpdateRequest:
            {
                const auto request = (const ObjectUpdateRequest *)  (*contentVector)[i];
                const auto id = request->updates()->id()->str();
                const auto animation = request->animation()->str();

                auto renderable = GlobalRenderableStore::INSTANCE()->getObjectById<Renderable>(id);
                if (renderable == nullptr) continue;

                renderable->setMatrix(request->updates()->matrix());
                const auto rot = request->updates()->rotation();
                renderable->setRotation({rot->x(), rot->y(), rot->z()});
                renderable->setScaling(request->updates()->scaling());
                renderable->setBoundingSphere(getBoundingSphere(request->updates()));

                if (!animation.empty()) {
                    static_cast<AnimatedModelMeshRenderable *>(renderable)->setCurrentAnimation(animation);
                    static_cast<AnimatedModelMeshRenderable *>(renderable)->setCurrentAnimationTime(request->animation_time());
                }

                break;
            }
            case MessageUnion_ObjectDebugRequest:
            {
                const auto request = (const ObjectDebugRequest *)  (*contentVector)[i];

                const auto id = request->id()->str();

                BoundingSphere boundingSphere;
                boundingSphere.radius = request->radius();
                boundingSphere.center = { request->center()->x(), request->center()->y(), request->center()->z() };

                BoundingBox bbox;
                bbox.min = { request->min()->x(), request->min()->y(), request->min()->z() };
                bbox.max = { request->max()->x(), request->max()->y(), request->max()->z() };

                if ((this->debugFlags & DEBUG_SPHERE) == DEBUG_SPHERE) {
                    const auto debugRenderable = GlobalRenderableStore::INSTANCE()->getObjectById<ColorMeshRenderable>(id+"-sphere");

                    if (debugRenderable == nullptr) {
                        auto sphereDebugGeom = Helper::createSphereColorMeshGeometry(request->radius()+0.001f, 20, 20, glm::vec4(1, 0, 0, 1));
                        sphereDebugGeom->sphere = boundingSphere;
                        auto sphereDebugeMeshRenderable = std::make_unique<ColorMeshRenderable>(id+"-sphere", sphereDebugGeom);
                        auto sphereDebugRenderable = GlobalRenderableStore::INSTANCE()->registerObject<ColorMeshRenderable>(sphereDebugeMeshRenderable);
                        sphereDebugRenderable->setMatrixForBoundingSphere(boundingSphere);
                        this->addDebugObjectsToBeRendered({sphereDebugRenderable});
                    } else {
                        auto sphereDebugGeom = Helper::createSphereColorMeshGeometry(request->radius()+0.001f, 20, 20, glm::vec4(1, 0, 0, 1));
                        debugRenderable->setMeshes(sphereDebugGeom->meshes);
                        debugRenderable->setBoundingSphere(boundingSphere);
                        debugRenderable->setMatrixForBoundingSphere(boundingSphere);
                        this->updateDebugObjectRenderable(debugRenderable);
                    }
                }

                if ((this->debugFlags & DEBUG_BBOX) == DEBUG_BBOX) {
                    const auto debugRenderable = GlobalRenderableStore::INSTANCE()->getObjectById<VertexMeshRenderable>(id+"-bbox");

                    if (debugRenderable == nullptr) {
                        auto bboxGeom = Helper::getBoundingBoxMeshGeometry(bbox);
                        bboxGeom->sphere = boundingSphere;
                        auto bboxMeshRenderable = std::make_unique<VertexMeshRenderable>(id+"-bbox", bboxGeom);
                        auto bboxRenderable = GlobalRenderableStore::INSTANCE()->registerObject<VertexMeshRenderable>(bboxMeshRenderable);
                        this->addDebugObjectsToBeRendered({bboxRenderable});
                    } else {
                        auto bboxGeom = Helper::getBoundingBoxMeshGeometry(bbox);
                        debugRenderable->setMeshes(bboxGeom->meshes);
                        debugRenderable->setBoundingSphere(boundingSphere);
                        this->updateDebugObjectRenderable(debugRenderable);
                    }
                }

                break;
            }
            case MessageUnion_NONE:
            case MessageUnion_ObjectCreateRequest:
            case MessageUnion_ObjectPropertiesUpdateRequest:
            {
                // server-side messages - no need to be handled
                break;
            }
        }
    }
}

bool Engine::startNetworking(const std::string ip, const uint16_t broadcastPort, const uint16_t requestPort)
{
    // connect to remote server
    if (this->client != nullptr) this->client->stop();

    this->client = std::make_unique<CommClient>(ip, broadcastPort, requestPort);
    if (this->client == nullptr) return false;

    auto handler = [this](void * message) { this->handleServerMessages(message);};
    if (!this->client->start(handler)) return false;

    return true;
}

void Engine::stopNetworking()
{
    if (this->client != nullptr) {
        this->client->stop();
        this->client = nullptr;
    }
}

void Engine::send(std::shared_ptr<flatbuffers::FlatBufferBuilder> & flatbufferBuilder, std::function<void (void *)> callback) const
{
    if (this->client == nullptr) return;

    this->client->sendBlocking(flatbufferBuilder, callback);
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

    SDL_SetWindowResizable(this->graphics->getSdlWindow(), SDL_FALSE);

    this->createRenderer();
    if (this->renderer == nullptr) return false;

    // establish singletons
    GlobalRenderableStore::INSTANCE();
    GlobalTextureStore::INSTANCE();

    if (!renderer->initRenderer()) return false;

    SDL_SetWindowResizable(this->graphics->getSdlWindow(), SDL_TRUE);

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
        this->renderer->setGpuCulling(false);
        logError("Your hardware has no combined compute/graphics queue. Falling back onto CPU frustum culling");
    }

    renderer = new Renderer(this->graphics, defaultPhysicalDevice, defaultQueueIndex, defaultComputeIndex);

    if (!renderer->isReady()) {
        logError("Failed to initialize Renderer!");
    }

    logInfo("Renderer is Ready");
}

void Engine::render(const std::chrono::high_resolution_clock::time_point & frameStart) {
    Camera::INSTANCE()->update(this);
    Camera::INSTANCE()->updateFrustum();

    bool wasRecording = this->renderer->isRecording();
    bool zeroFramesRecorded = this->renderer->getCachedFrames().empty();
    if (!wasRecording && zeroFramesRecorded) this->renderer->setRecording(true);

    bool addFrameToCache = this->renderer->isRecording();
    const uint64_t timeSinceLastFrameAddedToCache = this->renderer->getAccumulatedDeltaTime() - this->lastFrameAddedToCache;
    addFrameToCache = addFrameToCache && !this->renderer->isPaused() && timeSinceLastFrameAddedToCache > FRAME_RECORDING_INTERVAL;

    this->renderer->render(addFrameToCache);

    if (!wasRecording && zeroFramesRecorded) this->renderer->setRecording(false);

    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_span = now - frameStart;

    this->renderer->addDeltaTime(now, time_span.count());

    if (addFrameToCache) this->lastFrameAddedToCache = this->renderer->getAccumulatedDeltaTime();
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
                case SDL_KEYDOWN:
                    switch (e.key.keysym.scancode) {
                        case SDL_SCANCODE_1:
                        {
                            if (this->renderer->isPaused()) break;
                            this->setBackDrop(BLACK);
                            break;
                        }
                        case SDL_SCANCODE_2:
                        {
                            if (this->renderer->isPaused()) break;
                            this->setBackDrop(WHITE);
                            break;
                        }
                       case SDL_SCANCODE_KP_PLUS:
                        {
                            if (this->renderer->isPaused()) break;
                            this->adjustSunStrength(+0.1f);
                            break;
                        }
                        case SDL_SCANCODE_KP_MINUS:
                        {
                            if (this->renderer->isPaused()) break;
                            this->adjustSunStrength(-0.1f);
                            break;

                        }
                        case SDL_SCANCODE_W:
                        {
                            if (this->renderer->isPaused()) break;
                            this->camera->move(Camera::KeyPress::UP, true);
                            break;
                        }
                        case SDL_SCANCODE_S:
                        {
                            if (this->renderer->isPaused()) break;
                            this->camera->move(Camera::KeyPress::DOWN, true);
                            break;
                        }
                        case SDL_SCANCODE_A:
                        {
                            if (this->renderer->isPaused()) break;
                            this->camera->move(Camera::KeyPress::LEFT, true);
                            break;
                        }
                        case SDL_SCANCODE_D:
                        {
                            if (this->renderer->isPaused()) break;
                            this->camera->move(Camera::KeyPress::RIGHT, true);
                            break;
                        }
                        case SDL_SCANCODE_F:
                        {
                            if (this->renderer->isPaused()) break;
                            this->renderer->setShowWireFrame(!this->renderer->doesShowWireFrame());
                            break;
                        }
                        case SDL_SCANCODE_F5:
                        {
                            if (this->renderer->isPaused()) break;

                            // TODO: remove, for testing only
                            if (Camera::INSTANCE()->isInThirdPersonMode()) {
                                Camera::INSTANCE()->linkToRenderable(nullptr);
                            } else {
                                auto stego = GlobalRenderableStore::INSTANCE()->getObjectById<Renderable>("stego");
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
                            if (this->renderer->isPaused()) break;
                            this->camera->move(Camera::KeyPress::UP);
                            break;
                        }
                        case SDL_SCANCODE_S:
                        {
                            if (this->renderer->isPaused()) break;
                            this->camera->move(Camera::KeyPress::DOWN);
                            break;
                        }
                        case SDL_SCANCODE_A:
                        {
                            if (this->renderer->isPaused()) break;
                            this->camera->move(Camera::KeyPress::LEFT);
                            break;
                        }
                        case SDL_SCANCODE_D:
                        {
                            if (this->renderer->isPaused()) break;
                            this->camera->move(Camera::KeyPress::RIGHT);
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
                        case SDL_SCANCODE_R:
                        {
                            this->renderer->setRecording(!this->renderer->isRecording());
                            break;
                        }
                        case SDL_SCANCODE_SPACE:
                        {
                            if (!this->renderer->isPaused())
                            {
                                this->renderer->setRecording(false);
                                this->renderer->pause();
                            } else {
                                this->renderer->resume();
                            }
                            break;
                        }
                        default:
                            break;
                    };
                    break;
                case SDL_MOUSEMOTION:
                {
                    if (!this->renderer->isPaused() && SDL_GetRelativeMouseMode() == SDL_TRUE) {
                        this->camera->accumulateRotationDeltas(static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel));
                    }
                    break;
                }
                case SDL_MOUSEWHEEL:
                {
                    if (this->renderer->isPaused()) break;

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

const uint32_t Engine::getDebugFlags() const
{
    return this->debugFlags;
}

bool Engine::activateDebugging(const VkDeviceSize memorySize, const uint32_t debugFlags) {
    if (debugFlags == 0) return true;

    if ((debugFlags & DEBUG_SPHERE) == DEBUG_SPHERE)  {
        ColorMeshPipelineConfig colorMeshConf { this->renderer->usesGpuCulling() };
        colorMeshConf.useDeviceLocalForVertexSpace = false;
        colorMeshConf.useDeviceLocalForIndexSpace = false;
        colorMeshConf.reservedVertexSpace = memorySize;
        colorMeshConf.reservedIndexSpace = memorySize;

        CullPipelineConfig cullConf {};
        if (!this->createColorMeshPipeline(BOUNDING_SPHERE_PIPELINE, colorMeshConf, cullConf)) {
            logError("Failed to create BoundingSphere pipeline");
            return false;
        }
    }

    if ((debugFlags & DEBUG_BBOX) == DEBUG_BBOX) {
        VertexMeshPipelineConfig vertexMeshConf { this->renderer->usesGpuCulling() };
        vertexMeshConf.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        vertexMeshConf.useDeviceLocalForVertexSpace = false;
        vertexMeshConf.useDeviceLocalForIndexSpace = false;
        vertexMeshConf.reservedVertexSpace = memorySize;
        vertexMeshConf.reservedIndexSpace = memorySize;

        CullPipelineConfig cullConf { false };
        if (!this->createVertexMeshPipeline(BOUNDING_BOX_PIPELINE, vertexMeshConf, cullConf)) {
            logError("Failed to create BoundingSphere pipeline");
            return false;
        }
    }

    this->debugFlags = debugFlags;

    return true;
}

void Engine::deactivateDebugging() {
    this->debugFlags = 0;

    this->removePipeline(BOUNDING_SPHERE_PIPELINE);
    this->removePipeline(BOUNDING_BOX_PIPELINE);
}

bool Engine::createModelPipelines(const VkDeviceSize memorySizeModels, const VkDeviceSize memorySizeAnimatedModels) {
    bool ret = true;

    ModelMeshPipelineConfig modelConf { this->renderer->usesGpuCulling() };
    modelConf.reservedVertexSpace = memorySizeModels;
    modelConf.reservedIndexSpace = memorySizeModels;

    CullPipelineConfig cullConfModels {};
    ret = this->createModelMeshPipeline(MODELS_PIPELINE, modelConf, cullConfModels);

    AnimatedModelMeshPipelineConfig animatedModelConf {this->renderer->usesGpuCulling() };
    animatedModelConf.reservedVertexSpace = memorySizeAnimatedModels;
    animatedModelConf.reservedIndexSpace = memorySizeAnimatedModels;

    CullPipelineConfig cullConfAnimatedModels {};
    ret = this->createAnimatedModelMeshPipeline(ANIMATED_MODELS_PIPELINE, animatedModelConf, cullConfAnimatedModels);

    return ret;
}

bool Engine::createColorMeshPipelines(const VkDeviceSize memorySize, const VkDeviceSize memorySizeTextured) {
    bool ret = true;

    ColorMeshPipelineConfig colorMeshConf { this->renderer->usesGpuCulling() };
    colorMeshConf.useDeviceLocalForVertexSpace = false;
    colorMeshConf.useDeviceLocalForIndexSpace = false;
    colorMeshConf.reservedVertexSpace = memorySize;
    colorMeshConf.reservedIndexSpace = memorySize;

    CullPipelineConfig colorMeshCullConf {};
    ret = this->createColorMeshPipeline(COLOR_MESH_PIPELINE, colorMeshConf, colorMeshCullConf);

    TextureMeshPipelineConfig textureMeshConf { this->renderer->usesGpuCulling() };
    textureMeshConf.useDeviceLocalForVertexSpace = false;
    textureMeshConf.useDeviceLocalForIndexSpace = false;
    textureMeshConf.reservedVertexSpace = memorySizeTextured;
    textureMeshConf.reservedIndexSpace = memorySizeTextured;

    CullPipelineConfig textureMeshCullConf {};
    ret = this->createTextureMeshPipeline(TEXTURE_MESH_PIPELINE, textureMeshConf, textureMeshCullConf);

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

bool Engine::addDebugObjectsToBeRendered(const std::vector<ColorMeshRenderable *> & additionalDebugObjectsToBeRendered)
{
    auto pipe = this->getPipeline<ColorMeshPipeline>(BOUNDING_SPHERE_PIPELINE);
    if (pipe == nullptr) {
        logError("Engine lacks a suitable pipeline to render the objects!");
        return false;
    }

    return pipe->addObjectsToBeRendered(additionalDebugObjectsToBeRendered, true);
}

bool Engine::addDebugObjectsToBeRendered(const std::vector<VertexMeshRenderable *> & additionalDebugObjectsToBeRendered)
{
    auto pipe = this->getPipeline<VertexMeshPipeline>(BOUNDING_BOX_PIPELINE);
    if (pipe == nullptr) {
        logError("Engine lacks a suitable pipeline to render the objects!");
        return false;
    }

    return pipe->addObjectsToBeRendered(additionalDebugObjectsToBeRendered, true);
}

void Engine::updateDebugObjectRenderable(VertexMeshRenderable * renderable)
{
    if (renderable == nullptr) return;

    auto pipe = this->getPipeline<VertexMeshPipeline>(BOUNDING_BOX_PIPELINE);
    if (pipe == nullptr) return;

    pipe->updateVertexBufferForObjectWithId(renderable->getId());
    renderable->setDirty(true);
}

void Engine::updateDebugObjectRenderable(ColorMeshRenderable * renderable)
{
    if (renderable == nullptr) return;

    auto pipe = this->getPipeline<ColorMeshPipeline>(BOUNDING_SPHERE_PIPELINE);
    if (pipe == nullptr) return;

    pipe->updateVertexBufferForObjectWithId(renderable->getId());
    renderable->setDirty(true);
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
    if (this->quit) return false;

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
    if (this->renderer->usesGpuCulling() && optionalIndirectBufferIndex < 0) {
        logError("Could not create GPU culled Graphics Pipeline because there are no more free indirect buffers. Increase the limit.");
        return false;
    }

    graphicsConfig.indirectBufferIndex = optionalIndirectBufferIndex;
    if (!this->addPipeline<P>(name, graphicsConfig)) {
        this->removePipeline(name + "-cull");
        return false;
    }

    if (this->renderer->usesGpuCulling()) {
        cullConfig.indirectBufferIndex = optionalIndirectBufferIndex;

        auto graphicsPipelineToBeLinked = this->getPipeline<P>(name);
        if (graphicsPipelineToBeLinked == nullptr) return false;

        cullConfig.linkedGraphicsPipeline = MeshPipelineVariant { graphicsPipelineToBeLinked };

        if (!this->addPipeline<CullPipeline>(name + "-cull", cullConfig)) return false;
    }

    return true;
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

#include "includes/engine.h"

Engine::Engine(const std::string & appName, const std::string root, const uint32_t version) {
    logInfo("Creating Graphics Context...");
    this->graphics->initGraphics(appName, version);

    if(!this->graphics->isGraphicsActive()) {
        logError("Could not initialize Graphics Context");

        #ifdef __ANDROID__
                SDL_AndroidShowToast("Vulkan Not Supported", 1, 0, 0, 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(5000));
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
        case SKYBOX:
            return Engine::base / "skybox";
        case FONTS:
            return Engine::base / "fonts";
        case MAPS:
            return Engine::base / "maps";
        case ROOT:
        default:
            return Engine::base;
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

void Engine::init() {
    if(!this->graphics->isGraphicsActive()) return;

    this->createRenderer();
    if (this->renderer == nullptr) return;

    renderer->initRenderer();

    VkExtent2D windowSize = this->renderer->getSwapChainExtent();
    this->camera->setAspectRatio(static_cast<float>(windowSize.width) / windowSize.height);
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
    logInfo("Best Device:\t" + std::string(physicalProperties.deviceName));

    const int defaultQueueIndex = std::get<1>(bestRenderDevice);

    renderer = new Renderer(this->graphics, defaultPhysicalDevice, defaultQueueIndex);

    if (!renderer->isReady()) {
        logError("Failed to initialize Renderer!");
    }

    logInfo("Renderer is Ready");
}

void Engine::render(const std::chrono::high_resolution_clock::time_point & frameStart) {
    Camera::INSTANCE()->update(this->renderer->getDeltaTime());

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
                                if (isFullScreen && this->renderer->isMaximized()) {
                                    SDL_SetWindowFullscreen(this->graphics->getSdlWindow(), SDL_WINDOW_FULLSCREEN);
                                }
                            }
                    }
                    break;
                case SDL_KEYDOWN:
                    switch (e.key.keysym.scancode) {
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
        }

        this->render(frameStart);
    }

    SDL_StopTextInput();
}

bool Engine::addPipeline(std::unique_ptr<Pipeline> pipeline)
{
    if (this->renderer == nullptr) {
        logError("Engine requires a renderer instance!");
        return false;
    }

    return this->renderer->addPipeline(std::move(pipeline));
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


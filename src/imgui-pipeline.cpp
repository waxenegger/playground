#include "includes/pipeline.h"

ImGuiPipeline::ImGuiPipeline(const std::string name, Renderer * renderer) : GraphicsPipeline(name, renderer) { }

bool ImGuiPipeline::initPipeline(const PipelineConfig & config) {
    if (this->renderer == nullptr || !this->renderer->isReady()) {
        logError("Pipeline " + this->name + " requires a ready renderer instance!");
        return false;
    }

    this->config = std::move(static_cast<const ImGUIPipelineConfig &>(config));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    const uint32_t LIMIT = 100;

    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_SAMPLER, LIMIT);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LIMIT);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, LIMIT);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LIMIT);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, LIMIT);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, LIMIT);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LIMIT);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, LIMIT);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, LIMIT);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, LIMIT);
    this->descriptorPool.addResource(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, LIMIT);

    this->descriptorPool.createPool(this->renderer->getLogicalDevice(), LIMIT * this->descriptorPool.getNumberOfResources());

    if (!this->descriptorPool.isInitialized()) {
       logError("Failed to Create Descriptor Pool for ImGui!");
       return false;
    }

    // Setup Platform/Renderer backends
    if (!ImGui_ImplSDL2_InitForVulkan(this->renderer->getGraphicsContext()->getSdlWindow())) {
        logError("Failed to init ImGui Sdl!");
        return false;
    }

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = this->renderer->getGraphicsContext()->getVulkanInstance();
    init_info.PhysicalDevice = this->renderer->getPhysicalDevice();
    init_info.Device = this->renderer->getLogicalDevice();
    init_info.RenderPass = this->renderer->getRenderPass();
    init_info.QueueFamily = this->renderer->getGraphicsQueueIndex();
    init_info.Queue = this->renderer->getGraphicsQueue();
    init_info.DescriptorPool = this->descriptorPool.getPool();
    init_info.MinImageCount = this->renderer->getImageCount();
    init_info.ImageCount = this->renderer->getImageCount();

    if (!ImGui_ImplVulkan_Init(&init_info)) {
       logError("Failed to init ImGui for Vulkan!");
       return false;
    }

    const CommandPool & pool = this->renderer->getGraphicsCommandPool();
    const VkCommandBuffer buffer = pool.beginPrimaryCommandBuffer(this->renderer->getLogicalDevice());
    ImGui_ImplVulkan_CreateFontsTexture();
    pool.endCommandBuffer(buffer);
    pool.submitCommandBuffer(this->renderer->getLogicalDevice(), this->renderer->getGraphicsQueue(), buffer);
    ImGui::GetIO().Fonts->Build();
    ImGui_ImplVulkan_DestroyFontsTexture();

    if (!this->createAndLoadTextures()) return false;

    this->createFonts();

    return true;
}

bool ImGuiPipeline::createAndLoadTextures()
{
    int result = GlobalTextureStore::INSTANCE()->addTexture("recording-icon", "recording.png", true);
    if (result < 0) return false;

    const auto & recIconTex = GlobalTextureStore::INSTANCE()->getTextureByIndex(result);
    GlobalTextureStore::INSTANCE()->uploadTexturesToGPU(this->renderer);

    const auto & recIconImage = recIconTex->getTextureImage();
    if (recIconImage.isInitialized()) {
        this->recIconDesc = ImGui_ImplVulkan_AddTexture(recIconImage.getSampler(), recIconImage.getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    return true;
}


void ImGuiPipeline::createFonts()
{
    ImGuiIO &io = ImGui::GetIO();

    ImFontConfig config;
    config.SizePixels = 40;
    config.OversampleH = config.OversampleV = 1;
    config.PixelSnapH = true;

    this->defaultFont14Pixels = io.Fonts->AddFontDefault(&config);
}

bool ImGuiPipeline::createPipeline() {
    return (this->renderer != nullptr && this->renderer->isReady());
}

void ImGuiPipeline::draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex) {
    if (this->renderer == nullptr || !this->renderer->isReady() || !this->isEnabled()) return;

    const VkExtent2D extent = this->renderer->getSwapChainExtent();
    ImVec2 pos;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(extent.width, extent.height);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

    bool flag = true;

    if (ImGui::Begin("##mainWindow", &flag,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {

        ImGui::SetNextWindowPos(ImVec2(0.5f, 0.5f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(350.0f, 200.0f), ImGuiCond_Always);

        if (!this->renderer->isPaused() && ImGui::Begin("##debugContent", &flag,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {

            std::string fps = "FPS:\t" + std::to_string(this->renderer->getFrameRate());
            ImGui::Text("%s", fps.c_str());

            const auto camPos = Camera::INSTANCE()->getPosition();
            std::string pos = "Camera:\t" + std::to_string(camPos.x) + "|" + std::to_string(camPos.y) + "|" + std::to_string(camPos.z);
            ImGui::Text("%s", pos.c_str());

            const auto mem = this->renderer->getDeviceMemory();
            const std::string memInfo = "GPU:\t\t" + Helper::formatMemoryUsage(mem.used, true) + "/" + Helper::formatMemoryUsage(mem.total, true);
            ImGui::Text("%s", memInfo.c_str());

            const auto & memStats = this->renderer->getMemoryUsage();
            for (auto & m : memStats) {
                std::string memStat;
                if (m.vertexBufferTotal > 0) {
                    memStat = m.name + " Vertex:\t\t" + Helper::formatMemoryUsage(m.vertexBufferUsed, true) + "/" + Helper::formatMemoryUsage(m.vertexBufferTotal, true) + (m.vertexBufferUsesDeviceLocal ? "[GPU]" : "[HOST]");
                    ImGui::Text("%s", memStat.c_str());
                    if (m.indexBufferTotal > 0) {
                        memStat = m.name + " Index:\t\t" + Helper::formatMemoryUsage(m.indexBufferUsed, true) + "/" + Helper::formatMemoryUsage(m.indexBufferTotal, true) + (m.indexBufferUsesDeviceLocal ? "[GPU]" : "[HOST]");
                        ImGui::Text("%s", memStat.c_str());
                    }

                    if (m.instanceDataBufferTotal > 0) {
                        memStat = m.name + " Instance:\t" + Helper::formatMemoryUsage(m.instanceDataBufferUsed, true) + "/" + Helper::formatMemoryUsage(m.instanceDataBufferTotal, true) + "[HOST]";
                        ImGui::Text("%s", memStat.c_str());
                    }
                    if (m.meshDataBufferTotal > 0) {
                        memStat = m.name + " Mesh:\t\t" + Helper::formatMemoryUsage(m.meshDataBufferUsed, true) + "/" + Helper::formatMemoryUsage(m.meshDataBufferTotal, true) + "[HOST]";
                        ImGui::Text("%s", memStat.c_str());
                    }
                }

                if (m.computeBufferTotal > 0) {
                    memStat = m.name + " Compute:\t" + Helper::formatMemoryUsage(m.computeBufferUsed, true) + "/" + Helper::formatMemoryUsage(m.computeBufferTotal, true) + (m.computeBufferUsesDeviceLocal ? "[GPU]" : "[HOST]");
                    ImGui::Text("%s", memStat.c_str());
                }

                if (m.indirectBufferTotal > 0) {
                    memStat = m.name + " Indirect:\t" + Helper::formatMemoryUsage(m.indirectBufferTotal, true) + (m.indirectBufferUsesDeviceLocal ? "[GPU]" : "[HOST]");
                    ImGui::Text("%s", memStat.c_str());
                }
            }

            ImGui::End();
        }

        if (this->renderer->isRecording()) {
            bool show = this->lastRecordingShow >= 0;
            if (show) {
                pos = { io.DisplaySize.x-50, 10 };
                ImGui::SetCursorPos(pos);
                ImGui::Image((ImTextureID)this->recIconDesc, ImVec2(35, 35));

                if (this->lastRecordingShow > 1000) this->lastRecordingShow = -1000;
            }

            this->lastRecordingShow += this->renderer->getDeltaTime();
        } else {
            this->lastRecordingShow = 0;

            if (this->renderer->isPaused()) {
                const auto & cachedFrames = this->renderer->getCachedFrames();
                if (this->frameIndex >= cachedFrames.size()) this->frameIndex = 0;

                if (!cachedFrames.empty()) {
                    float diplayWidthHalf = io.DisplaySize.x/2;
                    pos = { diplayWidthHalf-diplayWidthHalf/2, io.DisplaySize.y/2 };

                    ImGui::PushFont(this->defaultFont14Pixels);
                    ImGui::SetCursorPos(pos);
                    ImGui::SetNextItemWidth(diplayWidthHalf);
                    ImGui::SliderInt("##debugFrames", &this->frameIndex, 0, cachedFrames.size()-1);
                    this->renderer->setCachedFrameIndex(this->frameIndex);
                    ImGui::PopFont();
                }
            }
        }

        ImGui::End();
    }

    ImGui::PopStyleVar();

    ImGui::EndFrame();
    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

bool ImGuiPipeline::canRender() const {
    return true;
}

void ImGuiPipeline::update() { }

ImGuiPipeline::~ImGuiPipeline() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}



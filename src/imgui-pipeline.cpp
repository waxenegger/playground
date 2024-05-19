#include "includes/engine.h"

ImGuiPipeline::ImGuiPipeline(const std::string name, Renderer * renderer) : GraphicsPipeline(name, renderer) { }

bool ImGuiPipeline::initPipeline(const PipelineConfig & config) {
    if (this->renderer == nullptr || !this->renderer->isReady()) return false;

    try {
        this->config = std::move(dynamic_cast<const ImGUIPipelineConfig &>(config));
    } catch (std::bad_cast ex) {
        logError("ImGuiPipeline needs instance of ImGUIPipelineConfig!");
        return false;
    }


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

    //TODO FIX + add keyboard and mouse interaction
    const CommandPool & pool = this->renderer->getGraphicsCommandPool();
    const VkCommandBuffer buffer = pool.beginPrimaryCommandBuffer(this->renderer->getLogicalDevice());
    ImGui_ImplVulkan_CreateFontsTexture();
    pool.endCommandBuffer(buffer);
    pool.submitCommandBuffer(this->renderer->getLogicalDevice(), this->renderer->getGraphicsQueue(), buffer);
    ImGui::GetIO().Fonts->Build();

    ImGui_ImplVulkan_DestroyFontsTexture();

    return true;
}

bool ImGuiPipeline::createPipeline() {
    return (this->renderer != nullptr && this->renderer->isReady());
}

void ImGuiPipeline::draw(const VkCommandBuffer & commandBuffer, const uint16_t commandBufferIndex) {
    if (this->renderer == nullptr || !this->renderer->isReady() || !this->isEnabled()) return;

    const VkExtent2D extent = this->renderer->getSwapChainExtent();

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

        const std::string fps = "FPS:\t" + std::to_string(this->renderer->getFrameRate());
        ImGui::Text("%s", fps.c_str());

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



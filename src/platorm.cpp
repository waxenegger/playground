#include "includes/graphics.h"

int start(int argc, char* argv []) {
    logInfo("Creating Graphics Context...");

    auto graphicsContext = std::make_unique<GraphicsContext>();
    graphicsContext->initGraphics(APP_NAME, VULKAN_VERSION);

    if(!graphicsContext->isGraphicsActive()) {
        logError("Could not initialize Graphics Context");

        #ifdef __ANDROID__
                SDL_AndroidShowToast("Vulkan Not Supported", 1, 0, 0, 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        #endif

        return -1;
    }

    graphicsContext->listPhysicalDevices();
    logInfo("Created Vulkan Context");

    return 0;
}

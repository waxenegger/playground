#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void createTestSpheres(Engine * engine, Vec4 color = {0.0f, 1.0f, 1.0f, 0.5f}, std::string texture = "") {

    if (engine == nullptr) return;

    CommBuilder builder;

    for (int i=-10;i<10;i+=5) {
        for (int j=-10;j<10;j+=5) {
            CommCenter::addObjectCreateSphereRequest(
                builder,
                "color-sphere-" + std::to_string(i) + "-" + std::to_string(j),
                {static_cast<float>(i),0,static_cast<float>(j)}, {0,0,0}, 1, 2.0f,
                color, texture);
        }
    }

    CommCenter::createMessage(builder, engine->getDebugFlags());
    engine->send(builder.builder);
}

void createModelTestObjects(Engine * engine) {

    if (engine == nullptr) return;

    CommBuilder builder;
    CommCenter::addObjectCreateModelRequest(builder, "cyborg", {0,30,0}, {0,0,0}, 1, "cyborg.obj", aiProcess_ConvertToLeftHanded);
    CommCenter::addObjectCreateModelRequest(builder, "nanosuit", {10,30,0}, {0,0,0}, 1, "nanosuit.obj", aiProcess_ConvertToLeftHanded);
    CommCenter::addObjectCreateModelRequest(builder, "contraption", {10,30,10}, {0,0,0}, 1, "contraption.obj");
    CommCenter::addObjectCreateModelRequest(builder, "stego", {10,10,10}, {0,0,0}, 1, "stegosaurs.gltf", aiProcess_ConvertToLeftHanded);
    CommCenter::addObjectCreateModelRequest(builder, "stego2", {0,10,0}, {0,0,0}, 1, "stegosaurs.gltf", aiProcess_ConvertToLeftHanded);
    CommCenter::addObjectCreateModelRequest(builder, "cesium", {0,15,0}, {0,0,0}, 1, "CesiumMan.gltf", true, aiProcess_ConvertToLeftHanded | aiProcess_ForceGenNormals);
    CommCenter::addObjectCreateModelRequest(builder, "bob", {10,15,10}, {0,0,0}, 1, "bob_lamp_update.md5mesh", aiProcess_ConvertToLeftHanded | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords);
    CommCenter::createMessage(builder, engine->getDebugFlags());
    engine->send(builder.builder);
}

std::function<void()> signalHandler0;

void signalHandler(int signal) {
    signalHandler0();
}

int start(int argc, char* argv []) {
    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    if (!engine->init()) return -1;

    signalHandler0 = [&engine] () -> void {
        engine->stop();
    };

    signal(SIGINT, signalHandler);

    if (!engine->startNetworking()) {
        logError("Failed to start Networking");
        return -1;
    }

    engine->createSkyboxPipeline();
    engine->createColorMeshPipelines(1000 * MEGA_BYTE, 1000* MEGA_BYTE);
    engine->createModelPipelines(100 * MEGA_BYTE, 100* MEGA_BYTE);

    //engine->activateDebugging(1000 * MEGA_BYTE, DEBUG_BOUNDING);

    engine->createGuiPipeline();

    if (engine->getRenderer()->hasAtLeastOneActivePipeline()) {
        const auto & asynJob = [&] () {
            createTestSpheres(engine.get(), Vec4(0.0f, 1.0f, 1.0f, 1.0f));
            createModelTestObjects(engine.get());
        };

        auto f = std::async(std::launch::async, asynJob);
    } else {
        logInfo("Warning: Engine has no pipeline to process work...");
    }

    engine->loop();

    return 0;

}

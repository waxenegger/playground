#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void createTestObjectsWithTextures(Engine * engine) {

    if (engine == nullptr) return;

    std::vector<TextureMeshRenderable *> renderables;

    GlobalTextureStore::INSTANCE()->uploadTexture("earth", "earth.png", engine->getRenderer(), true);

    for (int i= -100;i<100;i+=5) {
        for (int j= -100;j<100;j+=5) {
            auto sphereGeom = Helper::createSphereTextureMeshGeometry(2, 20, 20, "earth");
            auto sphereMeshRenderable = std::make_unique<TextureMeshRenderable>("texture-sphere-" + std::to_string(i) + "-" + std::to_string(j), sphereGeom);
            auto sphereRenderable = GlobalRenderableStore::INSTANCE()->registerObject<TextureMeshRenderable>(sphereMeshRenderable);
            renderables.emplace_back(sphereRenderable);
            sphereRenderable->setPosition({i, 20,j});
        }
    }

    engine->addObjectsToBeRendered(renderables);
}

void createTestObjectsWithoutTextures(Engine * engine) {

    if (engine == nullptr) return;

    std::vector<ColorMeshRenderable *> renderables;

    for (int i= -100;i<100;i+=5) {
        for (int j= -100;j<100;j+=5) {
            auto sphereGeom = Helper::createSphereColorMeshGeometry(2.0, 20, 20, glm::vec4(0,1,1, 1.0));
            auto sphereMeshRenderable = std::make_unique<ColorMeshRenderable>("color-sphere-" + std::to_string(i) + "-" + std::to_string(j), sphereGeom);
            auto sphereRenderable = GlobalRenderableStore::INSTANCE()->registerObject<ColorMeshRenderable>(sphereMeshRenderable);
            renderables.emplace_back(sphereRenderable);
            sphereRenderable->setPosition({i, 0,j});
        }
    }

    engine->addObjectsToBeRendered(renderables);
}

void createModelTestObjects(Engine * engine) {

    if (engine == nullptr) return;

    std::vector<ModelMeshRenderable *> renderables;

    const auto & cyborg  = Model::loadFromAssetsFolder("cyborg", "cyborg.obj", aiProcess_ConvertToLeftHanded);
    if (cyborg.has_value()) {
        ModelMeshRenderable * m = std::get<ModelMeshRenderable *>(cyborg.value());
        m->setPosition({0,30,0});
        renderables.emplace_back(m);
    }

    const auto & nanosuit = Model::loadFromAssetsFolder("nanosuit", "nanosuit.obj", aiProcess_ConvertToLeftHanded);
    if (nanosuit.has_value()) {
        ModelMeshRenderable * m = std::get<ModelMeshRenderable *>(nanosuit.value());
        m->setPosition({10,30,0});
        renderables.emplace_back(m);
    }

    const auto & contraption = Model::loadFromAssetsFolder("contraption", "contraption.obj");
    if (contraption.has_value()) {
        ModelMeshRenderable * m = std::get<ModelMeshRenderable *>(contraption.value());
        m->setPosition({10,30,10});
        renderables.emplace_back(m);
    }

    engine->addObjectsToBeRendered(renderables);

    std::vector<AnimatedModelMeshRenderable *> animatedRenderables;

    const auto & stegosaur = Model::loadFromAssetsFolder("stego", "stegosaurs.gltf", aiProcess_ConvertToLeftHanded);
    if (stegosaur.has_value()) {
        AnimatedModelMeshRenderable * m = std::get<AnimatedModelMeshRenderable *>(stegosaur.value());
        m->setPosition({10,10,0});
        m->setCurrentAnimation("run1");
        animatedRenderables.emplace_back(m);
    }

    const auto & stegosaur2 = Model::loadFromAssetsFolder("stego2", "stegosaurs.gltf", aiProcess_ConvertToLeftHanded);
    if (stegosaur2.has_value()) {
        AnimatedModelMeshRenderable * m = std::get<AnimatedModelMeshRenderable *>(stegosaur2.value());
        m->setPosition({0,10,0});
        m->setCurrentAnimation("run1");

        animatedRenderables.emplace_back(m);
    }

    const auto & cesium = Model::loadFromAssetsFolder("cesium", "CesiumMan.gltf", aiProcess_ConvertToLeftHanded | aiProcess_ForceGenNormals, true);
    if (cesium.has_value()) {
        AnimatedModelMeshRenderable * m = std::get<AnimatedModelMeshRenderable *>(cesium.value());
        m->setPosition({0,15,0});
        m->setScaling(1);
        m->rotate(90,0,0);
        animatedRenderables.emplace_back(m);
    }

    const auto & bob = Model::loadFromAssetsFolder("bob", "bob_lamp_update.md5mesh", aiProcess_ConvertToLeftHanded | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords);
    if (bob.has_value()) {
        AnimatedModelMeshRenderable * m = std::get<AnimatedModelMeshRenderable *>(bob.value());
        m->setPosition({10,10,10});
        m->rotate(90,0,180);

        animatedRenderables.emplace_back(m);
    }

    engine->addObjectsToBeRendered(animatedRenderables);
    engine->getRenderer()->forceNewTexturesUpload();
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
    engine->createColorMeshPipelines(100 * MEGA_BYTE, 100* MEGA_BYTE);
    engine->createModelPipelines(100 * MEGA_BYTE, 100* MEGA_BYTE);
    engine->createGuiPipeline();

    if (engine->getRenderer()->hasAtLeastOneActivePipeline()) {
        const auto & asynJob = [&] () {
            createTestObjectsWithTextures(engine.get());
            createTestObjectsWithoutTextures(engine.get());
            createModelTestObjects(engine.get());
        };

        auto f = std::async(std::launch::async, asynJob);
    } else {
        logInfo("Warning: Engine has no pipeline to process work...");
    }

    engine->loop();

    return 0;

}

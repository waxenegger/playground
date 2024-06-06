#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void createTestObjects(Engine * engine) {
    if (engine == nullptr) return;

    auto colorMeshPipeline = static_cast<ColorMeshPipeline *>(engine->getPipeline("colorMeshes"));
    std::vector<ColorMeshRenderable *> renderables;

    for (int i= -10;i<10;i+=5) {
        for (int j= -10;j<10;j+=5) {
            auto sphereGeom = Geometry::createSphereColorMeshGeometry(2, 20, 20, glm::vec4(0,1,0, 1));
            auto sphereMeshRenderable = std::make_unique<ColorMeshRenderable>(sphereGeom);
            auto sphereRenderable = GlobalRenderableStore::INSTANCE()->registerRenderable<ColorMeshRenderable>(sphereMeshRenderable);
            renderables.emplace_back(sphereRenderable);
            sphereRenderable->setPosition({i, 0,j});
        }
    }

    colorMeshPipeline->addObjectsToBeRendered(renderables);
}

int start(int argc, char* argv []) {

    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();

    engine->createSkyboxPipeline();

    ColorMeshPipelineConfig conf {};
    conf.useDeviceLocalForVertexSpace = true;
    conf.useDeviceLocalForIndexSpace = true;
    conf.reservedVertexSpace = 1500 * MEGA_BYTE;
    conf.reservedIndexSpace = 1500 * MEGA_BYTE;

    CullPipelineConfig cullConf {};
    cullConf.useDeviceLocalForComputeSpace = true;
    if (engine->createColorMeshPipeline("colorMeshes", conf, cullConf)) {
        if (engine->createDebugPipeline("colorMeshes", true, true))
            createTestObjects(engine.get());
    }

    engine->createGuiPipeline();

    engine->loop();

    return 0;

}

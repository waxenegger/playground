#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void createTestObjectsWithTextures(Engine * engine) {

    if (engine == nullptr) return;

    auto meshPipeline = engine->getPipeline<TextureMeshPipeline>("textureMeshes");
    if (meshPipeline == nullptr) return;

    std::vector<TextureMeshRenderable *> renderables;

    GlobalTextureStore::INSTANCE()->uploadTexture("earth", "earth.png",engine->getRenderer());

    for (int i= -10;i<10;i+=5) {
        for (int j= -10;j<10;j+=5) {
            auto sphereGeom = Helper::createSphereTextureMeshGeometry(2, 20, 20, "earth");
            auto sphereMeshRenderable = std::make_unique<TextureMeshRenderable>(sphereGeom);
            auto sphereRenderable = GlobalRenderableStore::INSTANCE()->registerRenderable<TextureMeshRenderable>(sphereMeshRenderable);
            renderables.emplace_back(sphereRenderable);
            sphereRenderable->setPosition({i, 0,j});
        }
    }

    meshPipeline->addObjectsToBeRendered(renderables);
}

void createTestObjectsWithoutTextures(Engine * engine) {

    if (engine == nullptr) return;

    auto meshPipeline = engine->getPipeline<ColorMeshPipeline>("colorMeshes");
    if (meshPipeline == nullptr) return;

    std::vector<ColorMeshRenderable *> renderables;

    for (int i= -5;i<5;i+=5) {
        for (int j= -5;j<5;j+=5) {
            auto sphereGeom = Helper::createSphereColorMeshGeometry(2.2, 20, 20, glm::vec4(0,1,1, 0.5));
            auto sphereMeshRenderable = std::make_unique<ColorMeshRenderable>(sphereGeom);
            auto sphereRenderable = GlobalRenderableStore::INSTANCE()->registerRenderable<ColorMeshRenderable>(sphereMeshRenderable);
            renderables.emplace_back(sphereRenderable);
            sphereRenderable->setPosition({i, 0,j});
        }
    }

    meshPipeline->addObjectsToBeRendered(renderables);
}

int start(int argc, char* argv []) {

    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();

    engine->createSkyboxPipeline();

    ColorMeshPipelineConfig colorConf {};
    colorConf.reservedVertexSpace = 500 * MEGA_BYTE;
    colorConf.reservedIndexSpace = 500 * MEGA_BYTE;

    TextureMeshPipelineConfig conf {};
    conf.reservedVertexSpace = 500 * MEGA_BYTE;
    conf.reservedIndexSpace = 500 * MEGA_BYTE;

    CullPipelineConfig cullConf {};
    if (engine->createTextureMeshPipeline("textureMeshes", conf, cullConf)) {
        //if (engine->createDebugPipeline("textureMeshes", true, true))
            createTestObjectsWithTextures(engine.get());
    }

    CullPipelineConfig cullConf2 {};
    if (engine->createColorMeshPipeline("colorMeshes", colorConf, cullConf2)) {
        //if (engine->createDebugPipeline("colorMeshes", true, true))
            createTestObjectsWithoutTextures(engine.get());
    }

    engine->createGuiPipeline();

    engine->loop();

    return 0;

}

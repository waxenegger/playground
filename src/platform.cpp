#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void createTestObjectsWithTextures(Engine * engine) {

    if (engine == nullptr) return;

    auto meshPipeline = engine->getPipeline<TextureMeshPipeline>("textureMeshes");
    if (meshPipeline == nullptr) return;

    std::vector<TextureMeshRenderable *> renderables;

    GlobalTextureStore::INSTANCE()->uploadTexture("earth", "earth.png",engine->getRenderer(), true);

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
            auto sphereGeom = Helper::createSphereColorMeshGeometry(2.2, 20, 20, glm::vec4(0,1,1, 1.0));
            auto sphereMeshRenderable = std::make_unique<ColorMeshRenderable>(sphereGeom);
            auto sphereRenderable = GlobalRenderableStore::INSTANCE()->registerRenderable<ColorMeshRenderable>(sphereMeshRenderable);
            renderables.emplace_back(sphereRenderable);
            sphereRenderable->setPosition({i, 0,j});
        }
    }

    meshPipeline->addObjectsToBeRendered(renderables);
}

void createModelTestObjects(Engine * engine) {

    if (engine == nullptr) return;

    auto meshPipeline = engine->getPipeline<ModelMeshPipeline>("modelMeshes");
    if (meshPipeline == nullptr) return;

    std::vector<ModelMeshRenderable *> renderables;

    const auto & cyborg  = Model::loadFromAssetsFolder("cyborg.obj", aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if (cyborg.has_value()) {
        ModelMeshRenderable * m = std::get<ModelMeshRenderable *>(cyborg.value());
        m->setPosition({0,10,0});
        renderables.push_back(m);
    }

    const auto & nanosuit = Model::loadFromAssetsFolder("nanosuit.obj", aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
    if (nanosuit.has_value()) {
        ModelMeshRenderable * m = std::get<ModelMeshRenderable *>(nanosuit.value());
        m->setPosition({10,10,0});
        renderables.push_back(m);
    }

    const auto & contraption = Model::loadFromAssetsFolder("contraption.obj");
    if (contraption.has_value()) {
        ModelMeshRenderable * m = std::get<ModelMeshRenderable *>(contraption.value());
        m->setPosition({10,30,10});
        renderables.push_back(m);
    }

    const auto & cesium = Model::loadFromAssetsFolder("CesiumMan.gltf", aiProcess_FlipUVs , true);
    if (cesium.has_value()) {
        ModelMeshRenderable * m = std::get<ModelMeshRenderable *>(cesium.value());
        m->setPosition({-10,20,-10});
        m->setScaling(2);
        m->rotate(-90,0,0);
        renderables.push_back(m);
    }

    const auto & stegosaur = Model::loadFromAssetsFolder("stegosaurs.gltf", aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
    if (stegosaur.has_value()) {
        ModelMeshRenderable * m = std::get<ModelMeshRenderable *>(stegosaur.value());
        m->setPosition({-10,20,10});
        m->setScaling(2);
        renderables.push_back(m);
    }

    meshPipeline->addObjectsToBeRendered(renderables);
}

int start(int argc, char* argv []) {

    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();

    engine->createSkyboxPipeline();

    TextureMeshPipelineConfig conf {};
    conf.reservedVertexSpace = 500 * MEGA_BYTE;
    conf.reservedIndexSpace = 500 * MEGA_BYTE;

    CullPipelineConfig cullConf {};
    if (engine->createTextureMeshPipeline("textureMeshes", conf, cullConf)) {
        //if (engine->createDebugPipeline("textureMeshes", true, true))
            createTestObjectsWithTextures(engine.get());
    }

    ColorMeshPipelineConfig colorConf {};
    colorConf.reservedVertexSpace = 500 * MEGA_BYTE;
    colorConf.reservedIndexSpace = 500 * MEGA_BYTE;

    CullPipelineConfig cullConf2 {};
    if (engine->createColorMeshPipeline("colorMeshes", colorConf, cullConf2)) {
        //if (engine->createDebugPipeline("colorMeshes", true, true))
            createTestObjectsWithoutTextures(engine.get());
    }

    ModelMeshPipelineConfig modelConf {};
    modelConf.reservedVertexSpace = 500 * MEGA_BYTE;
    modelConf.reservedIndexSpace = 500 * MEGA_BYTE;

    CullPipelineConfig cullConf3 {};
    cullConf3.reservedComputeSpace =  500 * MEGA_BYTE;
    if (engine->createModelMeshPipeline("modelMeshes", modelConf, cullConf3)) {
        //if (engine->createDebugPipeline("modelMeshes", true, true))
            createModelTestObjects(engine.get());
    }

    engine->createGuiPipeline();

    engine->loop();

    return 0;

}

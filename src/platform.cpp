#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void createTestObjectsWithTextures(Engine * engine) {

    if (engine == nullptr) return;

    auto meshPipeline = engine->getPipeline<TextureMeshPipeline>("textureMeshes");
    if (meshPipeline == nullptr) return;

    std::vector<TextureMeshRenderable *> renderables;

    GlobalTextureStore::INSTANCE()->uploadTexture("earth", "earth.png",engine->getRenderer(), true);

    for (int i= -100;i<100;i+=5) {
        for (int j= -100;j<100;j+=5) {
            auto sphereGeom = Helper::createSphereTextureMeshGeometry(2, 20, 20, "earth");
            auto sphereMeshRenderable = std::make_unique<TextureMeshRenderable>("texture-sphere-" + std::to_string(i) + "-" + std::to_string(j), sphereGeom);
            auto sphereRenderable = GlobalRenderableStore::INSTANCE()->registerRenderable<TextureMeshRenderable>(sphereMeshRenderable);
            renderables.emplace_back(sphereRenderable);
            sphereRenderable->setPosition({i, 20,j});
        }
    }

    meshPipeline->addObjectsToBeRendered(renderables);
}

void createTestObjectsWithoutTextures(Engine * engine) {

    if (engine == nullptr) return;

    auto meshPipeline = engine->getPipeline<ColorMeshPipeline>("colorMeshes");
    if (meshPipeline == nullptr) return;

    std::vector<ColorMeshRenderable *> renderables;

    for (int i= -100;i<100;i+=5) {
        for (int j= -100;j<100;j+=5) {
            auto sphereGeom = Helper::createSphereColorMeshGeometry(2.2, 20, 20, glm::vec4(0,1,1, 1.0));
            auto sphereMeshRenderable = std::make_unique<ColorMeshRenderable>("color-sphere-" + std::to_string(i) + "-" + std::to_string(j), sphereGeom);
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

    meshPipeline->addObjectsToBeRendered(renderables);

    std::vector<AnimatedModelMeshRenderable *> animatedRenderables;

    const auto & stegosaur = Model::loadFromAssetsFolder("stego", "stegosaurs.gltf", aiProcess_ConvertToLeftHanded);
    if (stegosaur.has_value()) {
        AnimatedModelMeshRenderable * m = std::get<AnimatedModelMeshRenderable *>(stegosaur.value());
        m->setPosition({10,10,0});
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

    const auto & bob = Model::loadFromAssetsFolder("bob", "bob_lamp_update.md5mesh", aiProcess_ConvertToLeftHanded | aiProcess_ForceGenNormals);
    if (bob.has_value()) {
        AnimatedModelMeshRenderable * m = std::get<AnimatedModelMeshRenderable *>(bob.value());
        m->setPosition({10,10,10});
        m->rotate(90,0,180);

        animatedRenderables.emplace_back(m);
    }

    auto animatedMeshPipeline = engine->getPipeline<AnimatedModelMeshPipeline>("animatedModelMeshes");
    if (animatedMeshPipeline == nullptr) return;

    animatedMeshPipeline->addObjectsToBeRendered(animatedRenderables);
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
            //createModelTestObjects(engine.get());
    }

    AnimatedModelMeshPipelineConfig animatedModelConf {};
    animatedModelConf.reservedVertexSpace = 500 * MEGA_BYTE;
    animatedModelConf.reservedIndexSpace = 500 * MEGA_BYTE;

    CullPipelineConfig cullConf4 {};
    cullConf4.reservedComputeSpace =  500 * MEGA_BYTE;
    if (engine->createAnimatedModelMeshPipeline("animatedModelMeshes", animatedModelConf, cullConf4)) {
        //if (engine->createDebugPipeline("animatedModelMeshes", true, true))
            createModelTestObjects(engine.get());
    }

    engine->createGuiPipeline();

    engine->loop();

    return 0;

}

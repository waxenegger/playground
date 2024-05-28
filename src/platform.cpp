#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void addSkyBoxPipeline(Engine * engine) {
    SkyboxPipelineConfig sky;
    engine->addPipeline<SkyboxPipeline>("sky", sky);
}

void addStaticGeometryPipelineAndTestGeometry(Engine * engine) {
    StaticObjectsColorMeshPipelineConfig conf;
    conf.reservedVertexSpace = 10 * MEGA_BYTE;
    conf.reservedIndexSpace = 10 * MEGA_BYTE;

    const auto & box = Geometry::createBoxColorMeshGeometry(15, 15, 15, glm::vec4(1,1,0, 0.5));
    const auto boxObject = new StaticColorMeshRenderable(box);
    GlobalRenderableStore::INSTANCE()->registerRenderable(boxObject);
    conf.objectsToBeRendered.push_back(boxObject);

    engine->addPipeline<StaticObjectsColorMeshPipeline>("static", conf);

    StaticObjectsColorVertexPipelineConfig normConf;
    normConf.reservedVertexSpace = 10 * MEGA_BYTE;
    normConf.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    Geometry::getNormalsFromColorMeshRenderables(std::vector<ColorMeshRenderable *> { boxObject }, normConf.objectsToBeRendered);
    Geometry::getBboxesFromRenderables(std::vector<Renderable *> { boxObject }, normConf.objectsToBeRendered);

    engine->addPipeline<StaticObjectsColorVertexPipeline>("debug", normConf);
}

void addDynamicGeometryPipelineAndTestGeometry(Engine * engine) {
    DynamicObjectsColorMeshPipelineConfig conf;
    conf.reservedVertexSpace = 10 * MEGA_BYTE;
    conf.reservedIndexSpace = 10 * MEGA_BYTE;

    const auto & sphere = Geometry::createSphereColorMeshGeometry(15, 50, 50, glm::vec4(0,1,0, 1));
    const auto sphereObject = new DynamicColorMeshRenderable(sphere);
    GlobalRenderableStore::INSTANCE()->registerRenderable(sphereObject);
    conf.objectsToBeRendered.push_back(sphereObject);

    engine->addPipeline<DynamicObjectsColorMeshPipeline>("dynamic", conf);

    auto p = static_cast<StaticObjectsColorVertexPipeline *>(engine->getPipeline("debug"));
    if (p != nullptr) {
        std::vector<StaticColorVerticesRenderable *> debugObjects;
            Geometry::getNormalsFromColorMeshRenderables(std::vector<ColorMeshRenderable *>{ sphereObject }, debugObjects);
            Geometry::getBboxesFromRenderables(std::vector<Renderable *> { sphereObject } , debugObjects);

        p->addObjectsToBeRenderer(debugObjects);
    }
}

void addGUIPipeline(Engine * engine) {
    ImGUIPipelineConfig guiConf;
    engine->addPipeline<ImGuiPipeline>("gui", guiConf);
}

int start(int argc, char* argv []) {

    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();

    addSkyBoxPipeline(engine.get());
    addStaticGeometryPipelineAndTestGeometry(engine.get());
    addDynamicGeometryPipelineAndTestGeometry(engine.get());
    addGUIPipeline(engine.get());

    engine->loop();

    return 0;

}

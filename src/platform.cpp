#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void addSkyBoxPipeline(Engine * engine) {
    SkyboxPipelineConfig sky;
    engine->addPipeline("sky", sky);
}

void addStaticGeometryPipelineAndTestGeometry(Engine * engine) {
    StaticObjectsColorVertexPipelineConfig conf;
    conf.reservedVertexSpace = 10000000;
    conf.reservedIndexSpace = 10000000;

    const auto & box = Geometry::createBox(15, 15, 15, glm::vec3(1,0,0));
    const auto boxObject = new StaticColorVerticesRenderable(box);
    GlobalRenderableStore::INSTANCE()->registerRenderable(boxObject);
    conf.objectsToBeRendered.push_back(boxObject);

    engine->addPipeline("static", conf);
}

void addDynamicGeometryPipelineAndTestGeometry(Engine * engine) {
    DynamicObjectsColorVertexPipelineConfig conf;
    conf.reservedVertexSpace = 10000000;
    conf.reservedIndexSpace = 10000000;

    const auto & sphere = Geometry::createSphere(15, 50, 50, glm::vec3(0,1,0));
    const auto sphereObject = new DynamicColorVerticesRenderable(sphere);
    GlobalRenderableStore::INSTANCE()->registerRenderable(sphereObject);
    conf.objectsToBeRendered.push_back(sphereObject);

    engine->addPipeline("dynamic", conf);
}

void addDebugPipeline(Engine * engine) {
    StaticObjectsColorVertexPipelineConfig normConf;
    normConf.reservedVertexSpace = 10000000;
    normConf.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    /*
    Geometry::getNormalsFromColorVertexRenderables<StaticColorVerticesRenderable>(staticConf.objectsToBeRendered, normConf.objectsToBeRendered);
    Geometry::getBboxesFromColorVertexRenderables<StaticColorVerticesRenderable>(staticConf.objectsToBeRendered, normConf.objectsToBeRendered);
    Geometry::getNormalsFromColorVertexRenderables<DynamicColorVerticesRenderable>(dynamicConf.objectsToBeRendered, normConf.objectsToBeRendered);
    Geometry::getBboxesFromColorVertexRenderables<DynamicColorVerticesRenderable>(dynamicConf.objectsToBeRendered, normConf.objectsToBeRendered);
    */

    engine->addPipeline("debug", normConf);
}

void addGUIPipeline(Engine * engine) {
    ImGUIPipelineConfig guiConf;
    engine->addPipeline("gui", guiConf);
}

int start(int argc, char* argv []) {

    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();

    addSkyBoxPipeline(engine.get());
    addStaticGeometryPipelineAndTestGeometry(engine.get());
    addDynamicGeometryPipelineAndTestGeometry(engine.get());
    addDebugPipeline(engine.get());
    addGUIPipeline(engine.get());

    engine->loop();

    return 0;

}

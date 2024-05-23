#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

void createTestPipelineConfig(StaticColorVertexPipelineConfig & config) {

    const auto & box = Geometry::createBox(15, 15, 15, glm::vec3(1,0,0));
    const auto boxObject = new StaticColorVerticesRenderable(box.first, box.second);
    GlobalRenderableStore::INSTANCE()->registerRenderable(boxObject);
    config.objectsToBeRendered.push_back(boxObject);

    const auto & sphere = Geometry::createSphere(15, 50, 50, glm::vec3(0,1,0));
    const auto sphereObject = new StaticColorVerticesRenderable(sphere);
    GlobalRenderableStore::INSTANCE()->registerRenderable(sphereObject);
    config.objectsToBeRendered.push_back(sphereObject);
}


int start(int argc, char* argv []) {

    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();


    SkyboxPipelineConfig sky;
    engine->addPipeline("sky", sky);

    StaticColorVertexPipelineConfig conf;
    conf.reservedVertexSpace = 10000000;
    conf.reservedIndexSpace = 10000000;
    createTestPipelineConfig(conf);
    engine->addPipeline("test", conf);

    StaticColorVertexPipelineConfig normConf;
    normConf.reservedVertexSpace = 10000000;
    normConf.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    Helper::extractNormalsFromColorVertexVector(conf.objectsToBeRendered, normConf.objectsToBeRendered);
    engine->addPipeline("normals", normConf);

    ImGUIPipelineConfig guiConf;
    engine->addPipeline("gui", guiConf);

    engine->loop();

    return 0;

}

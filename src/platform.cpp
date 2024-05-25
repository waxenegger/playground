#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void createTestPipelineConfig(ColorVertexPipelineConfig & config) {

    const auto & box = Geometry::createBox(15, 15, 15, glm::vec3(1,0,0));
    const auto boxObject = new ColorVerticesRenderable(box);
    GlobalRenderableStore::INSTANCE()->registerRenderable(boxObject);
    config.objectsToBeRendered.push_back(boxObject);

    const auto & sphere = Geometry::createSphere(15, 50, 50, glm::vec3(0,1,0));
    const auto sphereObject = new ColorVerticesRenderable(sphere);
    sphereObject->setScaling(0.01);
    GlobalRenderableStore::INSTANCE()->registerRenderable(sphereObject);
    config.objectsToBeRendered.push_back(sphereObject);
}


int start(int argc, char* argv []) {

    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();

    SkyboxPipelineConfig sky;
    engine->addPipeline("sky", sky);

    DynamicObjectsColorVertexPipelineConfig conf;
    conf.reservedVertexSpace = 10000000;
    conf.reservedIndexSpace = 10000000;
    createTestPipelineConfig(conf);
    engine->addPipeline("test", conf);

    ColorVertexPipelineConfig normConf;
    normConf.reservedVertexSpace = 10000000;
    normConf.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    Helper::getNormalsFromColorVertexRenderables(conf.objectsToBeRendered, normConf.objectsToBeRendered);
    Helper::getBboxesFromColorVertexRenderables(conf.objectsToBeRendered, normConf.objectsToBeRendered);

    engine->addPipeline("normals", normConf);

    ImGUIPipelineConfig guiConf;
    engine->addPipeline("gui", guiConf);

    engine->loop();

    return 0;

}

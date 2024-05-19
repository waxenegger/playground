#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

void createTestPipelineConfig(StaticColorVertexPipelineConfig & config) {
    const auto & box = Geometry::createBox(15, 15, 15, glm::vec3(255,0,0));
    config.colorVertices.insert(config.colorVertices.begin(), box.begin(), box.end());

    const auto & sphere = Geometry::createSphere(15, 15, 15, glm::vec3(0,255,0));
    config.colorVertices.insert(config.colorVertices.begin(), sphere.begin(), sphere.end());
}


int start(int argc, char* argv []) {

    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();

    SkyboxPipelineConfig sky;
    engine->addPipeline("sky", sky);

    StaticColorVertexPipelineConfig conf;
    createTestPipelineConfig(conf);
    engine->addPipeline("test", conf);

    StaticColorVertexPipelineConfig normConf;
    normConf.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    Helper::extractNormalsFromColorVertexVector(conf.colorVertices, normConf.colorVertices);
    engine->addPipeline("normals", normConf);

    ImGUIPipelineConfig guiConf;
    engine->addPipeline("gui", guiConf);

    engine->loop();

    return 0;

}

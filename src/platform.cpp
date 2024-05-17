#include "includes/pipelines.h"
#include "includes/geometry.h"

std::filesystem::path Engine::base  = "";

PipelineConfig createTestPipelineConfig() {
    PipelineConfig config;
    const auto & box = Geometry::createBox(15, 15, 15, glm::vec3(255,0,0));
    config.colorVertices.insert(config.colorVertices.begin(), box.begin(), box.end());

    const auto & sphere = Geometry::createSphere(15, 15, 15, glm::vec3(0,255,0));
    config.colorVertices.insert(config.colorVertices.begin(), sphere.begin(), sphere.end());

    return config;
}


int start(int argc, char* argv []) {

    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();

    // create mininal test pipeline for now
    std::unique_ptr<Pipeline> pipe = std::make_unique<StaticObjectsColorVertexPipeline>("test", engine->getRenderer());
    if (!pipe->addShader((Engine::getAppPath(SHADERS) / "test.vert.spv").string(), VK_SHADER_STAGE_VERTEX_BIT)) return -1;
    if (!pipe->addShader((Engine::getAppPath(SHADERS) / "test.frag.spv").string(), VK_SHADER_STAGE_FRAGMENT_BIT)) return -1;

    const PipelineConfig & conf = createTestPipelineConfig();

    if (!pipe->initPipeline(conf)) {
        logError("Failed to init Test Pipeline");
    }
    engine->addPipeline(std::move(pipe));

    // TODO: create another test pipeline for normals
    pipe = std::make_unique<StaticObjectsColorVertexPipeline>("normals", engine->getRenderer());
    if (!pipe->addShader((Engine::getAppPath(SHADERS) / "test.vert.spv").string(), VK_SHADER_STAGE_VERTEX_BIT)) return -1;
    if (!pipe->addShader((Engine::getAppPath(SHADERS) / "test.frag.spv").string(), VK_SHADER_STAGE_FRAGMENT_BIT)) return -1;

    PipelineConfig normConf;
    normConf.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    Helper::extractNormalsFromColorVertexVector(conf.colorVertices, normConf.colorVertices);

    if (!pipe->initPipeline(normConf)) {
        logError("Failed to init Normals Pipeline");
    }
    engine->addPipeline(std::move(pipe));

    std::unique_ptr<Pipeline> pipeline = std::make_unique<ImGuiPipeline>("gui", engine->getRenderer());
    if (pipeline->initPipeline(PipelineConfig {})) {
        logInfo("Initialized ImGui Pipeline");
    }
    engine->addPipeline(std::move(pipeline));

    engine->loop();

    return 0;

}

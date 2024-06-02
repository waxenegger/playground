#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void createPipelinesAndTestObjects(Engine * engine) {
    SkyboxPipelineConfig sky;
    engine->addPipeline<SkyboxPipeline>("sky", sky);

    ComputePipelineConfig compute;
    compute.reservedComputeSpace = 100 * MEGA_BYTE;

    ColorMeshPipelineConfig conf;
    conf.reservedVertexSpace = 2000 * MEGA_BYTE;
    conf.reservedIndexSpace = 2000 * MEGA_BYTE;

    ColorMeshPipelineConfig debug;
    debug.reservedVertexSpace = 1500 * MEGA_BYTE;
    debug.reservedIndexSpace = 10 * MEGA_BYTE;
    debug.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    /*
    auto bboxGeom = Geometry::getNormalsFromColorMeshRenderables(std::vector<ColorMeshRenderable *> { boxObject });
    if (bboxGeom != nullptr) debug.objectsToBeRendered.emplace_back(bboxGeom.release());
    auto normalsGeom = Geometry::getBboxesFromRenderables(std::vector<Renderable *> { boxObject });
    if (normalsGeom != nullptr) debug.objectsToBeRendered.emplace_back(normalsGeom.release());
    */

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    for (int i= -10;i<10;i+=5) {
        for (int j= -10;j<10;j+=5) {
            auto sphere = Geometry::createSphereColorMeshGeometry(2, 10, 10, glm::vec4(0,1,0, 1));
            auto sphereObject = new ColorMeshRenderable(sphere);
            GlobalRenderableStore::INSTANCE()->registerRenderable(sphereObject);
            conf.objectsToBeRendered.push_back(sphereObject);

            sphereObject->setPosition({i, 0,j});

            // TODO: accomodate non indexed linestrings as well
            auto normalsGeom = Geometry::getNormalsFromColorMeshRenderables(std::vector<ColorMeshRenderable *>{ sphereObject });
            if (normalsGeom != nullptr) debug.objectsToBeRendered.emplace_back(normalsGeom.release());

            auto bboxGeom = Geometry::getBboxesFromRenderables(std::vector<Renderable *> { sphereObject } );
            if (bboxGeom != nullptr) debug.objectsToBeRendered.emplace_back(bboxGeom.release());
        }
    }

    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    logInfo("First Loop: " + std::to_string(time_span.count()));


    if (USE_GPU_CULLING) engine->addPipeline<CullPipeline>("cull", compute);
    engine->addPipeline<ColorMeshPipeline>("test", conf);
    engine->addPipeline<ColorMeshPipeline>("debug", debug);

    ImGUIPipelineConfig guiConf;
    engine->addPipeline<ImGuiPipeline>("gui", guiConf);
}

int start(int argc, char* argv []) {

    const std::unique_ptr<Engine> engine = std::make_unique<Engine>(APP_NAME, argc > 1 ? argv[1] : "");

    if (!engine->isGraphicsActive()) return -1;

    engine->init();

    createPipelinesAndTestObjects(engine.get());

    engine->loop();

    return 0;

}

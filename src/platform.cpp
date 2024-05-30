#include "includes/engine.h"

std::filesystem::path Engine::base  = "";

// TODO: remove, for testing only
void addSkyBoxPipeline(Engine * engine) {
    SkyboxPipelineConfig sky;
    engine->addPipeline<SkyboxPipeline>("sky", sky);
}

void addStaticGeometryPipelineAndTestGeometry(Engine * engine) {
    ColorMeshPipelineConfig conf;
    conf.reservedVertexSpace = 10 * MEGA_BYTE;
    conf.reservedIndexSpace = 10 * MEGA_BYTE;

    const auto & box = Geometry::createBoxColorMeshGeometry(15, 10, 10, glm::vec4(1,1,0, 0.5));
    const auto boxObject = new ColorMeshRenderable(box);
    GlobalRenderableStore::INSTANCE()->registerRenderable(boxObject);
    conf.objectsToBeRendered.push_back(boxObject);

    engine->addPipeline<ColorMeshPipeline>("static", conf);

    ColorMeshPipelineConfig debugConf;
    debugConf.reservedVertexSpace = 10 * MEGA_BYTE;
    debugConf.reservedIndexSpace = 10 * MEGA_BYTE;
    debugConf.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    auto bboxGeom = Geometry::getNormalsFromColorMeshRenderables(std::vector<ColorMeshRenderable *> { boxObject });
    if (bboxGeom != nullptr) debugConf.objectsToBeRendered.emplace_back(bboxGeom.release());
    auto normalsGeom = Geometry::getBboxesFromRenderables(std::vector<Renderable *> { boxObject });
    if (normalsGeom != nullptr) debugConf.objectsToBeRendered.emplace_back(normalsGeom.release());

    engine->addPipeline<ColorMeshPipeline>("debug", debugConf);
}

void addDynamicGeometryPipelineAndTestGeometry(Engine * engine) {
    ColorMeshPipelineConfig conf;
    conf.reservedVertexSpace = 1000 * MEGA_BYTE;
    conf.reservedIndexSpace = 1000 * MEGA_BYTE;

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    auto p = static_cast<ColorMeshPipeline *>(engine->getPipeline("debug"));

    for (int i= -10;i<10;i+=5) {
        for (int j= -10;j<10;j+=5) {
            auto sphere = Geometry::createSphereColorMeshGeometry(2, 10, 10, glm::vec4(0,1,0, 0.5));
            auto sphereObject = new ColorMeshRenderable(sphere);
            GlobalRenderableStore::INSTANCE()->registerRenderable(sphereObject);
            conf.objectsToBeRendered.push_back(sphereObject);

            sphereObject->setPosition({i, 0,j});

            if (p != nullptr) {
                std::vector<ColorMeshRenderable *> debugObjects;
                auto normalsGeom = Geometry::getNormalsFromColorMeshRenderables(std::vector<ColorMeshRenderable *>{ sphereObject });
                if (normalsGeom != nullptr) debugObjects.emplace_back(normalsGeom.release());

                auto bboxGeom = Geometry::getBboxesFromRenderables(std::vector<Renderable *> { sphereObject } );
                if (bboxGeom != nullptr) debugObjects.emplace_back(bboxGeom.release());

                p->addObjectsToBeRenderer(debugObjects);
            }
        }
    }

    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    logInfo("First Loop: " + std::to_string(time_span.count()));

    engine->addPipeline<ColorMeshPipeline>("dynamic", conf);
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

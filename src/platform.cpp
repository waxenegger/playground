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

/*
void addDynamicGeometryPipelineAndTestGeometry(Engine * engine) {
    DynamicObjectsColorVertexPipelineConfig conf;
    conf.reservedVertexSpace = 3 * GIGA_BYTE;
    conf.reservedIndexSpace = 3 * GIGA_BYTE;

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    const auto & sphere = Geometry::createSphereColorVertexGeometry(2, 25, 25, glm::vec4(0,1,0, 1));

    uint64_t c=0;
    for (int i= -1000;i<1000;i+=5) {
        for (int j= -1000;j<1000;j+=5) {
            const auto sphereObject = new DynamicColorVerticesRenderable(sphere);
            GlobalRenderableStore::INSTANCE()->registerRenderable(sphereObject);
            conf.objectsToBeRendered.push_back(sphereObject);

            sphereObject->setPosition({i, 0,j});
            c++;
        }
    }

    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    logInfo("First Loop: " + std::to_string(time_span.count()));

    engine->addPipeline<DynamicObjectsColorVertexPipeline>("dynamic", conf);
}*/

void addDynamicGeometryPipelineAndTestGeometry(Engine * engine) {
    DynamicObjectsColorMeshPipelineConfig conf;
    conf.reservedVertexSpace = 3 * GIGA_BYTE;
    conf.reservedIndexSpace = 3 * GIGA_BYTE;

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    uint64_t c=0;
    for (int i= -1000;i<1000;i+=5) {
        for (int j= -1000;j<1000;j+=5) {
            auto sphere = Geometry::createSphereColorMeshGeometry(2, 25, 25, glm::vec4(0,1,0, 1));
            auto sphereObject = new DynamicColorMeshRenderable(sphere);
            GlobalRenderableStore::INSTANCE()->registerRenderable(sphereObject);
            conf.objectsToBeRendered.push_back(sphereObject);

            sphereObject->setPosition({i, 0,j});
            c++;
        }
    }

    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    logInfo("First Loop: " + std::to_string(time_span.count()));

    /*
    auto p = static_cast<StaticObjectsColorVertexPipeline *>(engine->getPipeline("debug"));
    if (p != nullptr) {
        std::vector<StaticColorVerticesRenderable *> debugObjects;
            Geometry::getNormalsFromColorMeshRenderables(std::vector<ColorMeshRenderable *>{ sphereObject }, debugObjects);
            Geometry::getBboxesFromRenderables(std::vector<Renderable *> { sphereObject } , debugObjects);

        p->addObjectsToBeRenderer(debugObjects);
    }*/

    engine->addPipeline<DynamicObjectsColorMeshPipeline>("dynamic", conf);
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

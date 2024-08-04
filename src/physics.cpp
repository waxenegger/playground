#include "includes/engine.h"

Physics::Physics() {}

void Physics::start()
{
    this->worker = std::thread { &Physics::work, this };
    this->worker.detach();
}

void Physics::stop()
{
    logInfo("Stopping Physics ...");
    this->quit = true;
}

void Physics::work()
{
    this->quit = false;

    while (!this->quit) {
        auto start = Communication::getTimeInMillis();

        const auto & collisions = this->performBroadPhaseCollisionCheck();
        this->checkAndResolveCollisions(collisions);

        auto end = Communication::getTimeInMillis();

        auto diff = end - start;

        //if (diff > 10) logInfo("Check Time " + std::to_string(end-start));
    }

    logInfo("Physics stopped.");
}

void Physics::addRenderablesToBeCollisionChecked(std::vector<Renderable *> renderables)
{
    const std::lock_guard<std::mutex> lock(this->additionMutex);

    for (auto r : renderables) {
        this->renderablesToBeChecked.push(r);
    }
}

ankerl::unordered_dense::map<std::string, std::set<Renderable *>> Physics::performBroadPhaseCollisionCheck()
//std::unordered_map<std::string, std::set<Renderable *>> Physics::performBroadPhaseCollisionCheck()
{
    std::vector<Renderable *> renderables;

    {
        const std::lock_guard<std::mutex> lock(this->additionMutex);

        while (!this->renderablesToBeChecked.empty()) {
            renderables.push_back(this->renderablesToBeChecked.front());
            this->renderablesToBeChecked.pop();
        }
    }

    //if (!renderables.empty()) logInfo("Checking " + std::to_string(renderables.size()));

    return SpatialRenderableStore::INSTANCE()->performBroadPhaseCollisionCheck(renderables);
}

void Physics::checkAndResolveCollisions(const ankerl::unordered_dense::map<std::string, std::set<Renderable *>> & collisions)
//void Physics::checkAndResolveCollisions(const std::unordered_map<std::string, std::set<Renderable *>> & collisions)
{
    //if (!collisions.empty()) logInfo("Resolving " + std::to_string(collisions.size()));

    for (auto c : collisions) {
        if (c.first.find("-pipe-debug-") != std::string::npos) continue;
        if (!c.second.empty()) {
            //logInfo("Detected collision of " + c.first + " with following:");
            for (auto r : c.second) {
                if (r->getName().find("-pipe-debug-") != std::string::npos) continue;
                //logInfo(r->getName());
            }

            // TODO: implement narrow phase collision checking
        }
    }
}

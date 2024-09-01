#include "includes/physics.h"

Physics::Physics() {}

void Physics::start()
{
    logInfo("Starting Physics ...");

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

        if (diff > 10) logInfo("Check Time " + std::to_string(end-start));
    }

    logInfo("Physics stopped.");
}

void Physics::addObjectsToBeUpdated(std::vector<PhysicsObject *> physicsObjects)
{
    const std::lock_guard<std::mutex> lock(this->additionMutex);

    for (auto r : physicsObjects) {
        this->objctsToBeUpdated.push(r);
    }
}

ankerl::unordered_dense::map<std::string, std::set<PhysicsObject *>> Physics::performBroadPhaseCollisionCheck()
{
    std::vector<PhysicsObject *> physicsObjects;

    {
        const std::lock_guard<std::mutex> lock(this->additionMutex);

        while (!this->objctsToBeUpdated.empty()) {
            physicsObjects.push_back(this->objctsToBeUpdated.front());
            this->objctsToBeUpdated.pop();
        }
    }

    return SpatialHashMap::INSTANCE()->performBroadPhaseCollisionCheck(physicsObjects);
}

void Physics::checkAndResolveCollisions(const ankerl::unordered_dense::map<std::string, std::set<PhysicsObject *>> & collisions)
{
    for (auto c : collisions) {
        if (!c.second.empty()) {
            logInfo("Detected collision of " + c.first + " with following:");
            for (auto r : c.second) {
                logInfo(r->getId());
            }

            // TODO: implement narrow phase collision checking
        }
    }
}

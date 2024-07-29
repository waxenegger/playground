#include "includes/engine.h"

Physics::Physics() {}

void Physics::start(Renderer * renderer)
{
    this->renderer = renderer;

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
        // TODO: first step: implement collision checking in here
    }

    logInfo("Physics stopped.");
}

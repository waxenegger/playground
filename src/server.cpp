#include "includes/server.h"

void logInfo(std::string message) {
    std::cout << message << std::endl;
}

void logError(std::string message) {
    std::cerr << message << std::endl;
}

bool stop = false;

void signalHandler(int signal) {
    stop = true;
}

static std::filesystem::path base;

int main(int argc, char* argv []) {
    const std::string root = argc > 1 ? argv[1] : "";
    const std::string ip = argc > 2 ? argv[2] : "127.0.0.1";

    base = root;

    if (!std::filesystem::exists(base)) {
        logError("App Directory " + base.string() + "does not exist!" );
        return -1;
    }

    if (base.empty()) {
        const std::filesystem::path cwd = std::filesystem::current_path();
        const std::filesystem::path cwdAppPath = cwd / "assets";
        logInfo("No App Directory Supplied. Assuming '" + cwdAppPath.string() + "' ...");

        if (std::filesystem::is_directory(cwdAppPath) && std::filesystem::exists(cwdAppPath)) {
            base = cwdAppPath;
        } else {
            logError("Sub folder 'assets' does not exist!");
            return -1;
        }
    }

    signal(SIGINT, signalHandler);

    std::unique_ptr<CommServer> server = std::make_unique<CommServer>(ip);
    std::unique_ptr<CommCenter> center = std::make_unique<CommCenter>();

    if (!server->start(std::bind(&CommCenter::queueMessages, center.get(), std::placeholders::_1))) return -1;

    GlobalPhysicsObjectStore::INSTANCE();
    SpatialHashMap::INSTANCE();

    std::unique_ptr<Physics> physics = std::make_unique<Physics>();
    physics->start();

    while(!stop) {
        const auto & nextMessage = center->getNextMessage();
        if (nextMessage != nullptr) {
            const auto contentVector = nextMessage->content();
            const auto contentVectorType = nextMessage->content_type();

            const uint32_t nrOfMessages = contentVector->size();
            logInfo("#messages: " + std::to_string(nrOfMessages));

            for (u_int32_t i=0;i<nrOfMessages;i++) {
                if ((const MessageUnion) (*contentVectorType)[i] == MessageUnion_CreateObject) {
                    const auto obj = (const CreateObject *) (*contentVector)[i];
                    const auto sphere = obj->object_as_CreateSphere();

                    const auto loco = obj->properties()->location();
                    const auto rot = obj->properties()->rotation();
                    logInfo(std::to_string(loco->x()) + "|" + std::to_string(loco->y()) + "|" + std::to_string(loco->z()));
                    logInfo(std::to_string(rot->x()) + "|" + std::to_string(rot->y()) + "|" + std::to_string(rot->z()));
                    logInfo("s " + std::to_string(obj->properties()->scale()));
                    logInfo("r " + std::to_string(sphere->radius()));
                }
            }
        }

        //server->send(CommCenter::createAckMessage());
        //Communication::sleepInMillis(10);
    }

    server->stop();
    physics->stop();

    return 0;
}

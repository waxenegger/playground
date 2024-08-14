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

int main(int argc, char* argv []) {
    const std::string root = argc > 1 ? argv[1] : "";
    const std::string ip = argc > 2 ? argv[2] : "127.0.0.1";

    ObjectFactory::base = root;

    if (!std::filesystem::exists(ObjectFactory::base)) {
        logError("App Directory " + ObjectFactory::base.string() + "does not exist!" );
        return -1;
    }

    if (ObjectFactory::base.empty()) {
        const std::filesystem::path cwd = std::filesystem::current_path();
        const std::filesystem::path cwdAppPath = cwd / "assets";
        logInfo("No App Directory Supplied. Assuming '" + cwdAppPath.string() + "' ...");

        if (std::filesystem::is_directory(cwdAppPath) && std::filesystem::exists(cwdAppPath)) {
            ObjectFactory::base = cwdAppPath;
        } else {
            logError("Sub folder 'assets' does not exist!");
            return -1;
        }
    }

    signal(SIGINT, signalHandler);

    std::unique_ptr<CommServer> server = std::make_unique<CommServer>(ip);
    std::unique_ptr<CommCenter> center = std::make_unique<CommCenter>();
    auto handler = [&center](void * message) {
        center->queueMessages(message);
    };

    if (!server->start(handler)) return -1;

    GlobalPhysicsObjectStore::INSTANCE();
    SpatialHashMap::INSTANCE();

    std::unique_ptr<Physics> physics = std::make_unique<Physics>();
    physics->start();

    while(!stop) {
        // get any queues messages and process them by delegation
        const auto nextMessage = center->getNextMessage();
        if (nextMessage != nullptr) {

            const auto m = GetMessage(static_cast<uint8_t *>(nextMessage));
            if (m == nullptr) continue;

            const auto contentVector = m->content();
            if (contentVector == nullptr) continue;

            const auto contentVectorType = m->content_type();
            if (contentVectorType == nullptr) continue;

            const uint32_t nrOfMessages = contentVector->size();
            logInfo("#messages: " + std::to_string(nrOfMessages));

            for (uint32_t i=0;i<nrOfMessages;i++) {
                const auto messageType = (const MessageUnion) (*contentVectorType)[i];
                if (messageType == MessageUnion_ObjectCreateRequest) {
                    const auto physicsObject = ObjectFactory::handleCreateObjectRequest((const ObjectCreateRequest *)  (*contentVector)[i]);
                    if (physicsObject != nullptr) {
                        CommBuilder builder;

                        if (ObjectFactory::handleCreateObjectResponse((const ObjectCreateRequest *) (*contentVector)[i], builder, physicsObject)) {
                            CommCenter::createMessage(builder);
                            server->send(builder.builder);
                        }
                    }
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

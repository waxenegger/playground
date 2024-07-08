#include "includes/world.h"

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
    const std::string ip = argc > 1 ? argv[1] : "127.0.0.1";
    const uint16_t udpPort = argc > 2 ? std::atoi(argv[2]) : 3000;
    const uint16_t tcpPort = argc > 3 ? std::atoi(argv[3]) : 3001;

    signal(SIGINT, signalHandler);

    std::unique_ptr<CommServer> server = std::make_unique<CommServer>(ip, udpPort, tcpPort);
    if (!server->start()) return -1;

    uint32_t n = 0;
    while(!stop) {
        server->send(std::to_string(n));
        n++;
        Communication::sleepInMillis(1000);
    }
    server->stop();

    return 0;
}

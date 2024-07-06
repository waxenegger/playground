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
    const uint16_t port = argc > 2 ? std::atoi(argv[2]) : 3000;

    signal(SIGINT, signalHandler);

    std::thread networking([ip, port] {
        startUdpRadio(stop, ip, port);
    });

    networking.join();

    return 0;
}

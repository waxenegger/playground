#include "includes/shared.h"

void logInfo(std::string message) {
    std::cout << message << std::endl;
}

void logError(std::string message) {
    std::cerr << message << std::endl;
}

int main(int argc, char* argv []) {
    return start(argc, argv);;
}

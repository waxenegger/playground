#ifdef __ANDROID__
    #include "includes/android.h"
    #include "includes/shared.h"

    #include <jni.h>

    void logInfo(std::string message) {
        LOG_INFO("%s", message.c_str());
    }

    void logError(std::string message) {
        LOG_ERROR("%s", message.c_str());
    }

    int main(int argc, char* argv []) {
        return start(argc, argv);
    }
#endif

#include "includes/communication.h"

bool startUdpRadio(const bool & stop, const std::string ip, const uint16_t port) {
    void * ctx = zmq_ctx_new();
    void * radio = zmq_socket(ctx, ZMQ_RADIO);

    const std::string address = "udp://" + ip + ":" + std::to_string(port);

    logInfo( "Starting UDP Radio @ " + address);

    int ret = zmq_connect(radio, address.c_str());
    if (ret== -1) {
        logError("Failed to start UDP Radio!");
        return false;
    }

    int message_num = 0;

    while (!stop){
        std::string data = std::to_string(message_num);

        zmq_msg_t msg;
        zmq_msg_init_data (&msg, (void *) data.c_str(), data.size(), NULL, NULL);
        zmq_msg_set_group(&msg, "playground");

        zmq_sendmsg(radio, &msg, 0);

        message_num++;
        sleep(100);
    }

    logInfo("Shutting down UDP Radio...");

    zmq_close(radio);
    zmq_ctx_term(ctx);

    return true;
};


#include "includes/communication.h"

Communication::Communication(const std::string ip, const uint16_t udpPort, const uint16_t tcpPort)
{
    this->udpAddress = "udp://" + ip + ":" + std::to_string(udpPort);
    this->tcpAddress = "tcp://" + ip + ":" + std::to_string(tcpPort);
}

void Communication::sleepInMillis(const uint32_t millis)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

uint64_t Communication::getTimeInMillis() {
    const std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    return now.count();
}

uint32_t Communication::getRandomUint32() {
    return Communication::distribution(Communication::default_random_engine);
}

bool CommClient::start()
{
    if (this->running) return true;

    this->running = true;
    std::thread listener([this] {
        void *ctx = zmq_ctx_new();
        void * dish = zmq_socket(ctx, ZMQ_DISH);

        //int maxWaitInMillis = 1000;
        //zmq_setsockopt(dish, ZMQ_RCVTIMEO, &maxWaitInMillis, sizeof(int));

        int ret = zmq_bind(dish, this->udpAddress.c_str());
        if (ret < 0) {
            this->running = false;
            logError("CommClient: Failed to bind UDP socket");
            return;
        }

        zmq_join(dish, "udp");

        logInfo("Listening to UDP traffic at: " + this->udpAddress);

        while (this->running){
            zmq_msg_t recv_msg;
            zmq_msg_init (&recv_msg);
            int size = zmq_recvmsg (dish, &recv_msg, ZMQ_DONTWAIT);
            if (size > 0) {
                logInfo("UDP Message received: " + std::string(static_cast<char *>(zmq_msg_data(&recv_msg)), size));
            }

            zmq_msg_close (&recv_msg);
        }

        logInfo("Stopped listening to UDP traffic.");

        zmq_close(dish);
        zmq_ctx_term(ctx);
    });

    listener.detach();

    return this->running;
}

std::string CommClient::sendBlocking(const std::string message, const bool waitForResponse)
{
    void * ctx = zmq_ctx_new();
    void * request = zmq_socket(ctx, ZMQ_REQ);

    //int maxWaitInMillis = 1000;
    //zmq_setsockopt(request, ZMQ_RCVTIMEO, &maxWaitInMillis, sizeof(int));

    // set identity
    auto now = Communication::getTimeInMillis();
    auto rnd = Communication::getRandomUint32();
    auto id = "REQ" + std::to_string(now) + std::to_string(rnd);

    zmq_setsockopt (request, ZMQ_IDENTITY, id.c_str(), id.size());

    int ret = zmq_connect(request, this->tcpAddress.c_str());
    if (ret < 0) {
        logError("CommClient: Failed to connect to TCP router");
        return "";
    }

    zmq_msg_t msg;
    zmq_msg_init_data (&msg, (void *) message.c_str(), message.size(), NULL, NULL);
    zmq_sendmsg(request, &msg, ZMQ_DONTWAIT);
    zmq_msg_close (&msg);

    // wait for reply
    std::string reply;

    if (waitForResponse) {
        zmq_msg_t recv_msg;
        zmq_msg_init (&recv_msg);
        int size = zmq_recvmsg (request, &recv_msg, 0);
        if (size > 0) {
            reply = std::string(static_cast<char *>(zmq_msg_data(&recv_msg)), size);
        }
        zmq_msg_close (&recv_msg);
    }

    zmq_close(request);
    zmq_ctx_term(ctx);

    return reply;
}

void CommClient::sendAsync(const std::string message, const bool waitForResponse, std::function<void (const std::string)> callback)
{
    auto wrappedBlockingFunction = [this, message, waitForResponse, callback] {
        const std::string resp = this->sendBlocking(message, waitForResponse);
        callback(resp);
    };

    auto asyncTask = std::async(std::launch::async, wrappedBlockingFunction);
    this->addAsyncTask(asyncTask);
}

void CommClient::addAsyncTask(std::future<void> & future) {
    // first get rid of finished ones
    std::vector<uint32_t > indicesForErase;

    uint32_t i=0;
    for (auto & f : this->pendingFutures) {
        if (f.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            indicesForErase.emplace_back(i);
        }
        i++;
    }

    if (!indicesForErase.empty()) {
        std::reverse(indicesForErase.begin(), indicesForErase.end());
        for (auto i : indicesForErase) {
            this->pendingFutures.erase(this->pendingFutures.begin()+i);
        }
    }

    this->pendingFutures.push_back(std::move(future));
}

void CommClient::stop()
{
    if (!this->running) return;

    logInfo("Shutting down CommClient ...");

    this->running = false;
}

bool CommServer::start()
{
    if (this->running) return true;

    if (!this->startUdp()) return false;

    if (!this->startTcp()) return false;

    return true;
}

bool CommServer::startUdp()
{
    if (this->running) return true;

    logInfo( "Trying to start UDP Radio at: " + this->udpAddress);

    this->udpContext = zmq_ctx_new();
    this->udpRadio = zmq_socket(this->udpContext, ZMQ_RADIO);
    if (this->udpRadio == nullptr) {
        logError("CommServer: Failed to create UDP socket");
        return false;
    }

    zmq_ctx_set(this->udpContext, ZMQ_IO_THREADS, 1);

    int max_threads = zmq_ctx_get (this->udpContext, ZMQ_IO_THREADS);
    logInfo("Max Threads: " + std::to_string(max_threads));
    int max_sockets = zmq_ctx_get (this->udpContext, ZMQ_MAX_SOCKETS);
    logInfo("Max Sockets: " + std::to_string(max_sockets));

    int ret = zmq_connect(this->udpRadio, this->udpAddress.c_str());
    if (ret== -1) {
        logError("Failed to connect UDP Radio!");
        return false;
    }

    logInfo( "UDP Radio broadcasting at: " + this->udpAddress);

    return true;
}

bool CommServer::startTcp() {
    if (this->running) return true;

    this->tcpContext = zmq_ctx_new();
    this->tcpPub = zmq_socket(this->tcpContext, ZMQ_ROUTER);
    if (this->tcpPub == nullptr) {
        logError("CommServer: Failed to create TCP socket");
        return false;
    }

    int ret = zmq_bind(this->tcpPub, this->tcpAddress.c_str());
    if (ret== -1) {
        logError("Failed to bin TCP Router!");
        return false;
    }

    this->running = true;
    std::thread listening([this] {
        logInfo( "TCP Router listening at: " + this->tcpAddress);

        while (this->running) {
            zmq_msg_t recv_msg;
            zmq_msg_init (&recv_msg);

            // peer indentity comes first
            int size = zmq_recvmsg (this->tcpPub, &recv_msg, 0);
            if (size > 0) {
                std::string clientId = std::string(static_cast<char *>(zmq_msg_data(&recv_msg)), size);

                // ignore empty delimiter
                zmq_recvmsg(this->tcpPub, &recv_msg, 0);

                // read actual message
                size = zmq_recvmsg (this->tcpPub, &recv_msg, 0);
                if (size > 0) {
                    logInfo("Router received: " + std::string(static_cast<char *>(zmq_msg_data(&recv_msg)), size));

                    std::string ack = "acknowledged";

                    zmq_msg_t msg;

                    zmq_msg_init_data (&msg, (void *) clientId.c_str(), clientId.size(), NULL, NULL);
                    zmq_sendmsg(this->tcpPub, &msg, ZMQ_SNDMORE);

                    zmq_msg_init_data (&msg, (void *) "", 0, NULL, NULL);
                    zmq_sendmsg(this->tcpPub, &msg, ZMQ_SNDMORE);

                    zmq_msg_init_data (&msg, (void *) ack.c_str(), ack.size(), NULL, NULL);
                    zmq_sendmsg(this->tcpPub, &msg, ZMQ_DONTWAIT);
                    zmq_msg_close (&msg);
                }
            }

            zmq_msg_close (&recv_msg);
        }

        zmq_close(this->tcpPub);
        this->tcpPub = nullptr;
        zmq_ctx_term(this->tcpContext);
        this->tcpContext = nullptr;

        logInfo( "TCP Router stopped listening");
    });

    listening.detach();

    return true;
}

void CommServer::send(const std::string message)
{
    if (!this->running || this->udpRadio == nullptr) return;

    zmq_msg_t msg;
    zmq_msg_init_data (&msg, (void *) message.c_str(), message.size(), NULL, NULL);
    zmq_msg_set_group(&msg, "udp");

    zmq_sendmsg(this->udpRadio, &msg, ZMQ_DONTWAIT);
}

void CommServer::stop()
{
    if (!this->running) return;

    logInfo("Shutting down CommServer ...");

    logInfo("Waiting 1s for things to finish ...");
    this->running = false;

    Communication::sleepInMillis(1000);

    if (this->udpRadio != nullptr) {
        zmq_close(this->udpRadio);
        this->udpRadio = nullptr;
    }

    if (this->udpContext != nullptr) {
        zmq_ctx_term(this->udpContext);
        this->udpContext = nullptr;
    }

    logInfo("CommServer shut down");
}

std::default_random_engine Communication::default_random_engine = std::default_random_engine();
std::uniform_int_distribution<int> Communication::distribution(1,10);

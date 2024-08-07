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
    uint64_t milliseconds_since_epoch =
        std::chrono::system_clock::now().time_since_epoch() /
        std::chrono::milliseconds(1);

    return milliseconds_since_epoch;
}

uint32_t Communication::getRandomUint32() {
    return Communication::distribution(Communication::default_random_engine);
}

void Communication::addAsyncTask(std::future<void> & future, const uint32_t thresholdForCleanup) {
    // first get rid of finished ones once threshold has been reached.
    if (this->pendingFutures.size() > thresholdForCleanup) {
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
    }

    this->pendingFutures.push_back(std::move(future));
}

bool CommClient::start(std::optional<const std::function<void(const Message *)>> messageHandler)
{
    if (this->running) return true;

    if (!messageHandler.has_value()) {
        logError("We need a message handler for the client to start!");
        return false;
    }

    this->running = true;
    std::thread listener([this, messageHandler] {
        void *ctx = zmq_ctx_new();
        void * dish = zmq_socket(ctx, ZMQ_DISH);

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
                messageHandler.value()(GetMessage(static_cast<uint8_t *>(zmq_msg_data(&recv_msg))));
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

void CommClient::sendBlocking(const std::shared_ptr<flatbuffers::FlatBufferBuilder> & message, std::optional<const std::function<void (const Message *)>> callback)
{
    void * ctx = zmq_ctx_new();
    void * request = zmq_socket(ctx, ZMQ_REQ);

    int timeOut = 5000;
    zmq_setsockopt(request, ZMQ_LINGER, &timeOut, sizeof(int));
    timeOut = 2000;
    zmq_setsockopt(request, ZMQ_RCVTIMEO, &timeOut, sizeof(int));

    // set identity
    auto now = Communication::getTimeInMillis();
    auto rnd = Communication::getRandomUint32();
    auto id = "REQ" + std::to_string(now) + std::to_string(rnd);

    zmq_setsockopt (request, ZMQ_IDENTITY, id.c_str(), id.size());

    int ret = zmq_connect(request, this->tcpAddress.c_str());
    if (ret < 0) {
        logError("CommClient: Failed to connect to TCP router");
        return;
    }

    zmq_msg_t msg;
    zmq_msg_init_data (&msg, (void *) message->GetBufferPointer(), message->GetSize(), NULL, NULL);
    zmq_sendmsg(request, &msg, 0);
    zmq_msg_close (&msg);

    // wait for reply (if callback)
    if (callback.has_value()) {
        zmq_msg_t recv_msg;
        zmq_msg_init (&recv_msg);
        int size = zmq_recvmsg (request, &recv_msg, 0);
        if (size > 0) {
            callback.value()(GetMessage(static_cast<uint8_t *>(zmq_msg_data(&recv_msg))));
        }
        zmq_msg_close (&recv_msg);
    }

    zmq_close(request);
    zmq_ctx_term(ctx);
}

void CommClient::sendAsync(const std::shared_ptr<flatbuffers::FlatBufferBuilder> & message, std::optional<const std::function<void (const Message *)>> callback)
{
    const auto & wrappedBlockingFunction = [this, message, callback] {
        this->sendBlocking(message, callback);
    };

    auto asyncTask = std::async(std::launch::async, wrappedBlockingFunction);
    this->addAsyncTask(asyncTask);
}

void CommClient::stop()
{
    if (!this->running) return;

    logInfo("Shutting down CommClient ...");

    this->running = false;
}

bool CommServer::start(std::optional<const std::function<void(const Message*)>> messageHandler)
{
    if (this->running) return true;

    if (!this->startUdp()) return false;
    if (!this->startTcp(messageHandler)) return false;

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

    int timeOut = 0;
    zmq_setsockopt(this->udpRadio, ZMQ_LINGER, &timeOut, sizeof(int));

    int ret = zmq_connect(this->udpRadio, this->udpAddress.c_str());
    if (ret== -1) {
        logError("Failed to connect UDP Radio!");
        return false;
    }

    logInfo( "UDP Radio broadcasting at: " + this->udpAddress);

    return true;
}

bool CommServer::startTcp(std::optional<const std::function<void(const Message*)>> messageHandler) {
    if (this->running) return true;

    this->tcpContext = zmq_ctx_new();
    this->tcpPub = zmq_socket(this->tcpContext, ZMQ_ROUTER);
    if (this->tcpPub == nullptr) {
        logError("CommServer: Failed to create TCP socket");
        return false;
    }

    int timeOut = 0;
    zmq_setsockopt(this->tcpPub, ZMQ_LINGER, &timeOut, sizeof(int));

    int ret = zmq_bind(this->tcpPub, this->tcpAddress.c_str());
    if (ret== -1) {
        logError("Failed to bin TCP Router!");
        return false;
    }

    const auto & ackReply = CommCenter::createAckMessage();

    this->running = true;
    std::thread listening([this, messageHandler, ackReply] {
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
                    if (messageHandler.has_value()) {
                        messageHandler.value()(GetMessage(static_cast<uint8_t *>(zmq_msg_data(&recv_msg))));
                    }

                    zmq_msg_t msg;

                    zmq_msg_init_data (&msg, (void *) clientId.c_str(), clientId.size(), NULL, NULL);
                    zmq_sendmsg(this->tcpPub, &msg, ZMQ_SNDMORE);

                    zmq_msg_init_data (&msg, (void *) "", 0, NULL, NULL);
                    zmq_sendmsg(this->tcpPub, &msg, ZMQ_SNDMORE);

                    zmq_msg_init_data (&msg, (void *) ackReply->GetBufferPointer(), ackReply->GetSize(), NULL, NULL);
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

void CommServer::sendBlocking(const std::shared_ptr<flatbuffers::FlatBufferBuilder> & message)
{
    zmq_msg_t msg;
    zmq_msg_init_data(&msg, static_cast<void *>(message->GetBufferPointer()), message->GetSize(), NULL, NULL);
    zmq_msg_set_group(&msg, "udp");
    zmq_sendmsg(this->udpRadio, &msg, ZMQ_DONTWAIT);
    zmq_msg_close (&msg);
}

void CommServer::send(const std::shared_ptr<flatbuffers::FlatBufferBuilder> & message)
{
    if (!this->running) return;

    this->sendBlocking(message);
}

void CommServer::sendAsync(const std::shared_ptr<flatbuffers::FlatBufferBuilder> & message)
{
    if (!this->running) return;

    const auto & wrappedBlockingFunction = [this, message] {
        this->sendBlocking(message);
    };

    auto asyncTask = std::async(std::launch::async, wrappedBlockingFunction);
    this->addAsyncTask(asyncTask, 0);
}

void CommServer::stop()
{
    if (!this->running) return;

    logInfo("Shutting down CommServer ...");

    logInfo("Waiting 1s for things to finish ...");
    this->running = false;

    logInfo("CommServer shut down");
}

void CommCenter::queueMessages(const Message * message)
{
    std::lock_guard(this->messageQueueMutex);
    this->messages.emplace(message);
}

const Message * CommCenter::getNextMessage()
{
    std::lock_guard(this->messageQueueMutex);

    if (this->messages.empty()) return nullptr;

    const auto & ret = this->messages.front();
    this->messages.pop();

    return ret;
}

const flatbuffers::Offset<ObjectProperties> CommCenter::createObjectProperties(CommBuilder & builder, const Vec3 location, const Vec3 rotation, const float scale)
{
    const auto objProps = CreateObjectProperties(
        *builder.builder,
        &location, &rotation,
        scale
    );

    return objProps;
}

const flatbuffers::Offset<CreateObject> CommCenter::createSphereObject(CommBuilder & builder, const flatbuffers::Offset<ObjectProperties> & properties, const float radius)
{
    const auto sphere = CreateCreateSphere(*builder.builder, radius);
    const auto sphereObject = CreateCreateObject(*builder.builder, properties, CreateObjectUnion_CreateSphere, sphere.Union());
    return sphereObject;
}

std::shared_ptr<flatbuffers::FlatBufferBuilder> CommCenter::createAckMessage()
{
    CommBuilder builder;
    builder.ack = true;

    return CommCenter::createMessage(builder);
}

std::shared_ptr<flatbuffers::FlatBufferBuilder> CommCenter::createMessage(CommBuilder & builder)
{
    const auto messageUnionTypes = builder.builder->CreateVector(builder.messageTypes);
    const auto messageUnions = builder.builder->CreateVector(builder.messages);

    const auto message = CreateMessage(*builder.builder, messageUnionTypes, messageUnions, builder.ack);

    builder.builder->Finish(message);

    return builder.builder;
}

void CommCenter::addCreateSphereObject(CommBuilder & builder, const Vec3 location, const Vec3 rotation, const float scale, const float radius)
{
    const auto & props = CommCenter::createObjectProperties(builder, location, rotation, scale);
    const auto & sphere = CommCenter::createSphereObject(builder, props, radius);

    builder.messageTypes.push_back(MessageUnion_CreateObject);
    builder.messages.push_back(sphere.Union());
}


std::default_random_engine Communication::default_random_engine = std::default_random_engine();
std::uniform_int_distribution<int> Communication::distribution(1,10);

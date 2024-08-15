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

bool CommClient::start(std::function<void(void*)> messageHandler)
{
    if (this->running) return true;

    if (!this->startUdp(messageHandler)) return false;

    return this->startTcp();
}

bool CommClient::startTcp()
{
    logInfo("CommClient: Connecting to TCP router...");

    this->tcpContext = zmq_ctx_new();
    this->tcpSocket = zmq_socket(this->tcpContext, ZMQ_REQ);

    int timeOut = 1000;
    zmq_setsockopt(this->tcpSocket, ZMQ_LINGER, &timeOut, sizeof(int));
    timeOut = 2000;
    zmq_setsockopt(this->tcpSocket, ZMQ_RCVTIMEO, &timeOut, sizeof(int));

    // set identity
    const auto now = Communication::getTimeInMillis();
    const auto rnd = Communication::getRandomUint32();
    const auto id = "REQ" + std::to_string(now) + std::to_string(rnd);

    zmq_setsockopt (this->tcpSocket, ZMQ_IDENTITY, id.c_str(), id.size());

    int ret = zmq_connect(this->tcpSocket, this->tcpAddress.c_str());
    if (ret < 0) {
        logError("CommClient: Failed to connect to TCP router");
        return false;
    }

    logInfo("CommClient: Connected to TCP router");

    return true;
}

bool CommClient::startUdp(std::function<void(void *)> messageHandler)
{
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
                void * dataReceived = zmq_msg_data(&recv_msg);
                void * dataCloned = malloc(size);
                memcpy(dataCloned, dataReceived, size);
                zmq_msg_close(&recv_msg);

                messageHandler(dataCloned);
            }
        }

        logInfo("Stopped listening to UDP traffic.");

        zmq_close(dish);
        zmq_ctx_term(ctx);
    });

    listener.detach();

    return this->running;
}

void CommClient::sendBlocking(std::shared_ptr<flatbuffers::FlatBufferBuilder> & message, std::function<void (void*)> callback)
{
    zmq_msg_t msg;
    int size = message->GetSize();

    void * dataCloned = malloc(size);
    memcpy(dataCloned, message->GetBufferPointer(), size);

    auto freeFn = [](void * s, void * h) {
        if (s != nullptr) free(s);
    };

    zmq_msg_init_data (&msg, dataCloned, size, freeFn, NULL);
    zmq_sendmsg(this->tcpSocket, &msg, ZMQ_DONTWAIT);

    // wait for reply (if callback)
    zmq_msg_t recv_msg;
    zmq_msg_init (&recv_msg);
    size = zmq_recvmsg (this->tcpSocket, &recv_msg, 0);
    if (size > 0) {
        void * dataReceived = zmq_msg_data(&recv_msg);
        void * dataCloned = malloc(size);
        memcpy(dataCloned, dataReceived, size);

        zmq_msg_close(&recv_msg);
        callback(dataCloned);
    }

    zmq_msg_close (&msg);
}

void CommClient::sendAsync(std::shared_ptr<flatbuffers::FlatBufferBuilder> & message, std::function<void (void*)> callback)
{
    const auto & wrappedBlockingFunction = [this, &message, callback]() {
        this->sendBlocking(message, callback);
    };

    auto asyncTask = std::async(std::launch::async, wrappedBlockingFunction);
    this->addAsyncTask(asyncTask, 10);
}

void CommClient::stop()
{
    if (!this->running) return;

    if (this->tcpSocket != nullptr) zmq_close(this->tcpSocket);
    if (this->tcpContext != nullptr) zmq_ctx_term(this->tcpContext);

    logInfo("Shutting down CommClient ...");

    this->running = false;
}

bool CommServer::start(std::function<void(void *)> messageHandler)
{
    if (this->running) return true;
    if (!this->startUdp()) return false;
    if (!this->startTcp(messageHandler)) return false;

    return true;
}

bool CommServer::startUdp()
{
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

bool CommServer::startTcp(std::function<void(void *)> messageHandler) {
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

    CommBuilder builder;
    CommCenter::createAckMessage(builder);

    this->running = true;
    std::thread listening([this, flatbuffers = builder.builder, messageHandler] {
        logInfo( "TCP Router listening at: " + this->tcpAddress);

        while (this->running) {
            zmq_msg_t recv_msg;
            zmq_msg_init (&recv_msg);

            // peer indentity comes first
            int size = zmq_recvmsg(this->tcpPub, &recv_msg, 0);
            if (size > 0) {
                std::string clientId = std::string(static_cast<char *>(zmq_msg_data(&recv_msg)), size);

                // ignore empty delimiter
                zmq_recvmsg(this->tcpPub, &recv_msg, 0);

                // read actual message
                size = zmq_recvmsg (this->tcpPub, &recv_msg, 0);
                if (size > 0) {
                    void * dataReceived = zmq_msg_data(&recv_msg);
                    void * dataCloned = malloc(size);
                    memcpy(dataCloned, dataReceived, size);

                    messageHandler(dataCloned);

                    zmq_msg_t msg;

                    zmq_msg_init_data (&msg, (void *) clientId.c_str(), clientId.size(), NULL, NULL);
                    zmq_sendmsg(this->tcpPub, &msg, ZMQ_SNDMORE);

                    zmq_msg_init_data (&msg, (void *) "", 0, NULL, NULL);
                    zmq_sendmsg(this->tcpPub, &msg, ZMQ_SNDMORE);

                    zmq_msg_init_data (&msg, (void *) flatbuffers->GetBufferPointer(), flatbuffers->GetSize(), NULL, NULL);
                    zmq_sendmsg(this->tcpPub, &msg, ZMQ_DONTWAIT);

                    zmq_msg_close (&msg);
                }
            }

            zmq_msg_close(&recv_msg);
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

void CommServer::sendBlocking(std::shared_ptr<flatbuffers::FlatBufferBuilder> & message)
{
    zmq_msg_t msg;
    const int size = message->GetSize();

    void * clonedData = malloc(size);
    memcpy(clonedData, message->GetBufferPointer(), size);

    auto freeFn = [](void * s, void * h) {
        if (s != nullptr) free(s);
    };

    zmq_msg_init_data(&msg, clonedData, size, freeFn, NULL);
    zmq_msg_set_group(&msg, "udp");
    zmq_sendmsg(this->udpRadio, &msg, ZMQ_DONTWAIT);
    zmq_msg_close (&msg);
}

void CommServer::send(std::shared_ptr<flatbuffers::FlatBufferBuilder> & message)
{
    if (!this->running) return;

    this->sendBlocking(message);
}

void CommServer::sendAsync(std::shared_ptr<flatbuffers::FlatBufferBuilder> & message)
{
    if (!this->running) return;

    const auto & wrappedBlockingFunction = [this, &message] {
        this->sendBlocking(message);
    };

    auto asyncTask = std::async(std::launch::async, wrappedBlockingFunction);
    this->addAsyncTask(asyncTask, 10);
}

void CommServer::stop()
{
    if (!this->running) return;

    logInfo("Shutting down CommServer ...");

    logInfo("Waiting 1s for things to finish ...");
    this->running = false;

    logInfo("CommServer shut down");
}

void CommCenter::queueMessages(void * message)
{
    this->messages.emplace(message);
}

void * CommCenter::getNextMessage()
{
    if (this->messages.empty()) return nullptr;

    const auto & ret = this->messages.front();
    this->messages.pop();

    return ret;
}

const flatbuffers::Offset<ObjectProperties> CommCenter::createObjectProperties(CommBuilder & builder, const std::string id, const Vec3 location, const Vec3 rotation, const float scale)
{
    const auto objProps = CreateObjectProperties(
        *builder.builder,
        builder.builder->CreateString(id),
        &location, &rotation,
        scale
    );

    return objProps;
}

const flatbuffers::Offset<UpdatedObjectProperties> CommCenter::createUpdatesObjectProperties(CommBuilder & builder, const std::string id, const float radius, const Vec3 center, const std::array<Vec4, 4> columns)
{
    const auto matrix = CreateMatrix(
        *builder.builder,
        &columns[0],
        &columns[0],
        &columns[0],
        &columns[0]
    );

    const auto updatedObjProps = CreateUpdatedObjectProperties(
        *builder.builder,
        builder.builder->CreateString(id),
        radius,
        &center,
        matrix
    );

    return updatedObjProps;
}

void CommCenter::createAckMessage(CommBuilder & builder, const bool ack)
{
    builder.ack = ack;

    CommCenter::createMessage(builder);
}

void CommCenter::createMessage(CommBuilder & builder)
{
    const auto messageUnionTypes = builder.builder->CreateVector(builder.messageTypes);
    const auto messageUnions = builder.builder->CreateVector(builder.messages);

    const auto message = CreateMessage(*builder.builder, messageUnionTypes, messageUnions, builder.ack);

    builder.builder->Finish(message);
}

void CommCenter::addObjectCreateSphereRequest(CommBuilder & builder, const std::string id, const Vec3 location, const Vec3 rotation, const float scale, const float radius, const std::string texture)
{
    const auto & props = CommCenter::createObjectProperties(builder, id, location, rotation, scale);
    const auto sphere = CreateSphereCreateRequest(*builder.builder, radius, builder.builder->CreateString(texture));
    const auto sphereObject = CreateObjectCreateRequest(*builder.builder, props, ObjectCreateRequestUnion_SphereCreateRequest, sphere.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateRequest);
    builder.messages.push_back(sphereObject.Union());
}

void CommCenter::addObjectCreateBoxRequest(CommBuilder & builder, const std::string id, const Vec3 location, const Vec3 rotation, const float scale, const float width, const float height, const float depth, const std::string texture)
{
    const auto & props = CommCenter::createObjectProperties(builder, id, location, rotation, scale);

    const auto box = CreateBoxCreateRequest(*builder.builder, width, height, depth, builder.builder->CreateString(texture));
    const auto boxObject = CreateObjectCreateRequest(*builder.builder, props, ObjectCreateRequestUnion_BoxCreateRequest, box.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateRequest);
    builder.messages.push_back(boxObject.Union());
}

void CommCenter::addObjectCreateModelRequest(CommBuilder & builder, const std::string id, const Vec3 location, const Vec3 rotation, const float scale, const std::string file)
{
    const auto & props = CommCenter::createObjectProperties(builder, id, location, rotation, scale);

    const auto model = CreateModelCreateRequest(*builder.builder, builder.builder->CreateString(file));
    const auto modelObject = CreateObjectCreateRequest(*builder.builder, props, ObjectCreateRequestUnion_ModelCreateRequest, model.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateRequest);
    builder.messages.push_back(modelObject.Union());
}

void CommCenter::addObjectCreateAndUpdateSphereRequest(CommBuilder & builder, const std::string id, const float boundingSphereRadius, const Vec3 boundingSphereCenter, const std::array<Vec4, 4> columns, const float radius, const std::string texture)
{
    const auto & updateProps = CommCenter::createUpdatesObjectProperties(builder, id, boundingSphereRadius, boundingSphereCenter, columns);
    const auto sphere = CreateSphereUpdateRequest(*builder.builder, updateProps, radius, builder.builder->CreateString(texture));
    const auto createAndUpdateSphere = CreateObjectCreateAndUpdateRequest(*builder.builder, ObjectUpdateRequestUnion_SphereUpdateRequest, sphere.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateAndUpdateRequest);
    builder.messages.push_back(createAndUpdateSphere.Union());
}

void CommCenter::addObjectCreateAndUpdateBoxRequest(CommBuilder & builder, const std::string id, const float boundingSphereRadius, const Vec3 boundingSphereCenter, const std::array<Vec4, 4> columns, const float width, const float height, const float depth, const std::string texture)
{
    const auto & updateProps = CommCenter::createUpdatesObjectProperties(builder, id, boundingSphereRadius, boundingSphereCenter, columns);
    const auto box = CreateBoxUpdateRequest(*builder.builder, updateProps, width, height, depth, builder.builder->CreateString(texture));
    const auto createAndUpdateBox = CreateObjectCreateAndUpdateRequest(*builder.builder, ObjectUpdateRequestUnion_BoxUpdateRequest, box.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateAndUpdateRequest);
    builder.messages.push_back(createAndUpdateBox.Union());
}

void CommCenter::addObjectCreateAndUpdateModelRequest(CommBuilder & builder, const std::string id, const float boundingSphereRadius, const Vec3 boundingSphereCenter, const std::array<Vec4, 4> columns, const std::string file)
{
    const auto & updateProps = CommCenter::createUpdatesObjectProperties(builder, id, boundingSphereRadius, boundingSphereCenter, columns);
    const auto model = CreateModelUpdateRequest(*builder.builder, updateProps, builder.builder->CreateString(file));
    const auto createAndUpdateModel = CreateObjectCreateAndUpdateRequest(*builder.builder, ObjectUpdateRequestUnion_ModelUpdateRequest, model.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateAndUpdateRequest);
    builder.messages.push_back(createAndUpdateModel.Union());
}

void CommCenter::addObjectUpdateRequest(CommBuilder & builder, const std::string id, const float boundingSphereRadius, const Vec3 boundingSphereCenter, const std::array<Vec4, 4> columns)
{
    const auto & updateProps = CommCenter::createUpdatesObjectProperties(builder, id, boundingSphereRadius, boundingSphereCenter, columns);
    const auto update = CreateObjectUpdateRequest(*builder.builder, updateProps);

    builder.messageTypes.push_back(MessageUnion_ObjectUpdateRequest);
    builder.messages.push_back(update.Union());
}


std::default_random_engine Communication::default_random_engine = std::default_random_engine();
std::uniform_int_distribution<int> Communication::distribution(1,10);

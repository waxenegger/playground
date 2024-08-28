#include "includes/communication.h"

Communication::Communication(const std::string ip, const uint16_t broadcastPort, const uint16_t requestPort)
{
    this->broadcastAddress = "tcp://" + ip + ":" + std::to_string(broadcastPort);
    this->requestAddress = "tcp://" + ip + ":" + std::to_string(requestPort);
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

    int ret = zmq_connect(this->tcpSocket, this->requestAddress.c_str());
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

        int ret = zmq_bind(dish, this->broadcastAddress.c_str());
        if (ret < 0) {
            this->running = false;
            logError("CommClient: Failed to bind broadcast socket");
            return;
        }

        zmq_join(dish, "broadcast");

        logInfo("Listening to broadcast traffic at: " + this->broadcastAddress);

        while (this->running){
            zmq_msg_t recv_msg;
            zmq_msg_init (&recv_msg);
            int size = zmq_recvmsg (dish, &recv_msg, 0);

            if (size > 0) {
                void * dataReceived = zmq_msg_data(&recv_msg);
                void * dataCloned = malloc(size);
                memcpy(dataCloned, dataReceived, size);
                zmq_msg_close(&recv_msg);

                messageHandler(dataCloned);
            }
        }

        logInfo("Stopped listening to broadcast traffic.");

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
    if (!this->startBroadcast()) return false;
    if (!this->startRequestListener(messageHandler)) return false;

    return true;
}

bool CommServer::startBroadcast()
{
    logInfo( "Trying to start broadcast at: " + this->broadcastAddress);

    this->broadcastContext = zmq_ctx_new();
    this->broadcastRadio = zmq_socket(this->broadcastContext, ZMQ_RADIO);
    if (this->broadcastRadio == nullptr) {
        logError("CommServer: Failed to create broadcast socket");
        return false;
    }

    zmq_ctx_set(this->broadcastContext, ZMQ_IO_THREADS, 1);

    int max_threads = zmq_ctx_get (this->broadcastContext, ZMQ_IO_THREADS);
    logInfo("Max Threads: " + std::to_string(max_threads));
    int max_sockets = zmq_ctx_get (this->broadcastContext, ZMQ_MAX_SOCKETS);
    logInfo("Max Sockets: " + std::to_string(max_sockets));

    int timeOut = 1000;
    zmq_setsockopt(this->broadcastRadio, ZMQ_LINGER, &timeOut, sizeof(int));

    int ret = zmq_connect(this->broadcastRadio, this->broadcastAddress.c_str());
    if (ret== -1) {
        logError("Failed to connect broadcast!");
        return false;
    }

    logInfo( "Broadcasting at: " + this->broadcastAddress);

    return true;
}

bool CommServer::startRequestListener(std::function<void(void *)> messageHandler) {
    this->requestListenerContext = zmq_ctx_new();
    this->requestListener = zmq_socket(this->requestListenerContext, ZMQ_ROUTER);
    if (this->requestListener == nullptr) {
        logError("CommServer: Failed to create TCP socket");
        return false;
    }

    int timeOut = 1000;
    zmq_setsockopt(this->requestListener, ZMQ_LINGER, &timeOut, sizeof(int));

    int ret = zmq_bind(this->requestListener, this->requestAddress.c_str());
    if (ret== -1) {
        logError("Failed to bin TCP Router!");
        return false;
    }

    CommBuilder builder;
    CommCenter::createAckMessage(builder);

    this->running = true;
    std::thread listening([this, flatbuffers = builder.builder, messageHandler] {
        logInfo( "TCP Router listening at: " + this->requestAddress);

        while (this->running) {
            zmq_msg_t recv_msg;
            zmq_msg_init (&recv_msg);

            // peer indentity comes first
            int size = zmq_recvmsg(this->requestListener, &recv_msg, 0);
            if (size > 0) {
                std::string clientId = std::string(static_cast<char *>(zmq_msg_data(&recv_msg)), size);

                // ignore empty delimiter
                zmq_recvmsg(this->requestListener, &recv_msg, 0);

                // read actual message
                size = zmq_recvmsg (this->requestListener, &recv_msg, 0);
                if (size > 0) {
                    void * dataReceived = zmq_msg_data(&recv_msg);
                    void * dataCloned = malloc(size);
                    memcpy(dataCloned, dataReceived, size);

                    messageHandler(dataCloned);

                    zmq_msg_t msg;

                    zmq_msg_init_data (&msg, (void *) clientId.c_str(), clientId.size(), NULL, NULL);
                    zmq_sendmsg(this->requestListener, &msg, ZMQ_SNDMORE);

                    zmq_msg_init_data (&msg, (void *) "", 0, NULL, NULL);
                    zmq_sendmsg(this->requestListener, &msg, ZMQ_SNDMORE);

                    zmq_msg_init_data (&msg, (void *) flatbuffers->GetBufferPointer(), flatbuffers->GetSize(), NULL, NULL);
                    zmq_sendmsg(this->requestListener, &msg, ZMQ_DONTWAIT);

                    zmq_msg_close (&msg);
                }
            }

            zmq_msg_close(&recv_msg);
        }

        zmq_close(this->requestListener);
        this->requestListener = nullptr;
        zmq_ctx_term(this->requestListenerContext);
        this->requestListenerContext = nullptr;

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
    zmq_msg_set_group(&msg, "broadcast");
    zmq_sendmsg(this->broadcastRadio, &msg, ZMQ_DONTWAIT);
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

const flatbuffers::Offset<UpdatedObjectProperties> CommCenter::createUpdatesObjectProperties(CommBuilder & builder, const std::string id, const float radius, const Vec3 center, const std::array<Vec4, 4> columns, const Vec3 rotation, const float scaling)
{
    const auto matrix = CreateMatrix(
        *builder.builder,
        &columns[0],
        &columns[1],
        &columns[2],
        &columns[3]
    );

    const auto updatedObjProps = CreateUpdatedObjectProperties(
        *builder.builder,
        builder.builder->CreateString(id),
        radius,
        &center,
        matrix,
        &rotation,
        scaling
    );

    return updatedObjProps;
}

void CommCenter::createAckMessage(CommBuilder & builder, const bool ack, const uint32_t debugFlags)
{
    builder.ack = ack;
    builder.debugFlags = debugFlags;
    CommCenter::createMessage(builder, debugFlags);
}

void CommCenter::createMessage(CommBuilder & builder, const uint32_t debugFlags)
{
    builder.debugFlags = debugFlags;

    const auto messageUnionTypes = builder.builder->CreateVector(builder.messageTypes);
    const auto messageUnions = builder.builder->CreateVector(builder.messages);

    const auto message = CreateMessage(*builder.builder, builder.ack,  builder.debugFlags, messageUnionTypes, messageUnions);

    builder.builder->Finish(message);
}

void CommCenter::addObjectCreateSphereRequest(CommBuilder & builder, const std::string id, const Vec3 location, const Vec3 rotation, const float scale, const float radius, const Vec4 color, const std::string texture)
{
    const auto & props = CommCenter::createObjectProperties(builder, id, location, rotation, scale);
    const auto sphere = CreateSphereCreateRequest(*builder.builder, radius, &color, builder.builder->CreateString(texture));
    const auto sphereObject = CreateObjectCreateRequest(*builder.builder, props, ObjectCreateRequestUnion_SphereCreateRequest, sphere.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateRequest);
    builder.messages.push_back(sphereObject.Union());
}

void CommCenter::addObjectCreateBoxRequest(CommBuilder & builder, const std::string id, const Vec3 location, const Vec3 rotation, const float scale, const float width, const float height, const float depth, const Vec4 color, const std::string texture)
{
    const auto & props = CommCenter::createObjectProperties(builder, id, location, rotation, scale);

    const auto box = CreateBoxCreateRequest(*builder.builder, width, height, depth, &color, builder.builder->CreateString(texture));
    const auto boxObject = CreateObjectCreateRequest(*builder.builder, props, ObjectCreateRequestUnion_BoxCreateRequest, box.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateRequest);
    builder.messages.push_back(boxObject.Union());
}

void CommCenter::addObjectCreateModelRequest(CommBuilder & builder, const std::string id, const Vec3 location, const Vec3 rotation, const float scale, const std::string file, const uint32_t flags, const bool useFirstChildAsRoot)
{
    const auto & props = CommCenter::createObjectProperties(builder, id, location, rotation, scale);

    const auto model = CreateModelCreateRequest(*builder.builder, builder.builder->CreateString(file), flags, useFirstChildAsRoot);
    const auto modelObject = CreateObjectCreateRequest(*builder.builder, props, ObjectCreateRequestUnion_ModelCreateRequest, model.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateRequest);
    builder.messages.push_back(modelObject.Union());
}

void CommCenter::addObjectCreateAndUpdateSphereRequest(CommBuilder & builder, const std::string id, const float boundingSphereRadius, const Vec3 boundingSphereCenter, const std::array<Vec4, 4> columns, const Vec3 rotation, const float scale, const float radius, const Vec4 color, const std::string texture)
{
    const auto & updateProps = CommCenter::createUpdatesObjectProperties(builder, id, boundingSphereRadius, boundingSphereCenter, columns, rotation, scale);
    const auto sphere = CreateSphereUpdateRequest(*builder.builder, updateProps, radius, &color, builder.builder->CreateString(texture));
    const auto createAndUpdateSphere = CreateObjectCreateAndUpdateRequest(*builder.builder, ObjectUpdateRequestUnion_SphereUpdateRequest, sphere.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateAndUpdateRequest);
    builder.messages.push_back(createAndUpdateSphere.Union());
}

void CommCenter::addObjectCreateAndUpdateBoxRequest(CommBuilder & builder, const std::string id, const float boundingSphereRadius, const Vec3 boundingSphereCenter, const std::array<Vec4, 4> columns, const Vec3 rotation, const float scale, const float width, const float height, const float depth, const Vec4 color, const std::string texture)
{
    const auto & updateProps = CommCenter::createUpdatesObjectProperties(builder, id, boundingSphereRadius, boundingSphereCenter, columns, rotation, scale);
    const auto box = CreateBoxUpdateRequest(*builder.builder, updateProps, width, height, depth, &color, builder.builder->CreateString(texture));
    const auto createAndUpdateBox = CreateObjectCreateAndUpdateRequest(*builder.builder, ObjectUpdateRequestUnion_BoxUpdateRequest, box.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateAndUpdateRequest);
    builder.messages.push_back(createAndUpdateBox.Union());
}

void CommCenter::addObjectCreateAndUpdateModelRequest(CommBuilder & builder, const std::string id, const float boundingSphereRadius, const Vec3 boundingSphereCenter, const std::array<Vec4, 4> columns, const Vec3 rotation, const float scale, const std::string file, const std::string animation, const float animatonTime, const uint32_t flags, const bool useFirstChildAsRoot)
{
    const auto & updateProps = CommCenter::createUpdatesObjectProperties(builder, id, boundingSphereRadius, boundingSphereCenter, columns, rotation, scale);
    const auto model = CreateModelUpdateRequest(*builder.builder, updateProps, builder.builder->CreateString(file), builder.builder->CreateString(animation), animatonTime, flags, useFirstChildAsRoot);
    const auto createAndUpdateModel = CreateObjectCreateAndUpdateRequest(*builder.builder, ObjectUpdateRequestUnion_ModelUpdateRequest, model.Union());

    builder.messageTypes.push_back(MessageUnion_ObjectCreateAndUpdateRequest);
    builder.messages.push_back(createAndUpdateModel.Union());
}

void CommCenter::addObjectUpdateRequest(CommBuilder & builder, const std::string id, const float boundingSphereRadius, const Vec3 boundingSphereCenter, const std::array<Vec4, 4> columns, const Vec3 rotation, const float scale, const std::string animation, const float animationTime)
{
    const auto & updateProps = CommCenter::createUpdatesObjectProperties(builder, id, boundingSphereRadius, boundingSphereCenter, columns, rotation, scale);
    const auto update = CreateObjectUpdateRequest(*builder.builder, updateProps, builder.builder->CreateString(animation), animationTime);

    builder.messageTypes.push_back(MessageUnion_ObjectUpdateRequest);
    builder.messages.push_back(update.Union());
}

void CommCenter::addObjectDebugRequest(CommBuilder & builder, const std::string id, const float boundingSphereRadius, const Vec3 boundingSphereCenter, const Vec3 bboxMin, const Vec3 bboxMax)
{
    const auto debug = CreateObjectDebugRequest(*builder.builder, builder.builder->CreateString(id), boundingSphereRadius, &boundingSphereCenter, &bboxMin, &bboxMax);

    builder.messageTypes.push_back(MessageUnion_ObjectDebugRequest);
    builder.messages.push_back(debug.Union());
}

void CommCenter::addObjectPropertiesUpdateRequest(CommBuilder& builder, const std::string id, const Vec3 position, const Vec3 rotation, const float scaling, const std::string animation, const float animationTime)
{
    const auto update = CreateObjectPropertiesUpdateRequest(*builder.builder, builder.builder->CreateString(id), &position, &rotation, scaling, builder.builder->CreateString(animation), animationTime);

    builder.messageTypes.push_back(MessageUnion_ObjectPropertiesUpdateRequest);
    builder.messages.push_back(update.Union());
}


std::default_random_engine Communication::default_random_engine = std::default_random_engine();
std::uniform_int_distribution<int> Communication::distribution(1,10);

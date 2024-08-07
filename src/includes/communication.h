#ifndef SRC_INCLUDES_COMMUNICATION_INCL_H_
#define SRC_INCLUDES_COMMUNICATION_INCL_H_

#include "logging.h"
#include <zmq.h>
#include "message.h"

#include <queue>
#include <chrono>
#include <thread>
#include <functional>
#include <future>
#include <random>
#include <string.h>

class Communication {
    private:
        static std::default_random_engine default_random_engine;
        static std::uniform_int_distribution<int> distribution;

        std::vector<std::future<void>> pendingFutures;
    protected:
        bool running = false;

        std::string udpAddress;
        std::string tcpAddress;

        void addAsyncTask(std::future<void> & future, const uint32_t thresholdForCleanup = 0);

    public:
        Communication(const Communication&) = delete;
        Communication& operator=(const Communication &) = delete;
        Communication(Communication &&) = delete;
        Communication & operator=(Communication) = delete;

        Communication(const std::string ip, const uint16_t udpPort = 3000, const uint16_t tcpPort = 3001);

        virtual bool start(std::optional<const std::function<void(const Message*)>> messageHandler = std::nullopt) = 0;
        virtual void stop() = 0;

        static void sleepInMillis(const uint32_t millis);
        static uint64_t getTimeInMillis();
        static uint32_t getRandomUint32();

        virtual ~Communication() {};
};

class CommClient : public Communication {
    public:
        CommClient(const CommClient&) = delete;
        CommClient& operator=(const CommClient &) = delete;
        CommClient(CommClient &&) = delete;
        CommClient & operator=(CommClient) = delete;

        CommClient(const std::string ip, const uint16_t udpPort = 3000, const uint16_t tcpPort = 3001) : Communication(ip, udpPort, tcpPort) {};

        void sendBlocking(const std::shared_ptr<flatbuffers::FlatBufferBuilder> & message, std::optional<const std::function<void (const Message*)>> callback = std::nullopt);
        void sendAsync(const std::shared_ptr<flatbuffers::FlatBufferBuilder> & message, std::optional<const std::function<void (const Message*)>> callback = std::nullopt);

        bool start(std::optional<const std::function<void(const Message*)>> messageHandler = std::nullopt);
        void stop();
};

class CommServer : public Communication {
    private:
        void * udpContext = nullptr;
        void * udpRadio = nullptr;

        void * tcpContext = nullptr;
        void * tcpPub = nullptr;

        bool startUdp();
        bool startTcp(std::optional<const std::function<void(const Message*)>> messageHandler);
        void sendBlocking(const std::shared_ptr<flatbuffers::FlatBufferBuilder> & message);

    public:
        CommServer(const CommServer&) = delete;
        CommServer& operator=(const CommServer &) = delete;
        CommServer(CommServer &&) = delete;
        CommServer & operator=(CommServer) = delete;

        CommServer(const std::string ip, const uint16_t udpPort = 3000, const uint16_t tcpPort = 3001) : Communication(ip, udpPort, tcpPort) {};

        void send(const std::shared_ptr<flatbuffers::FlatBufferBuilder> & message);
        void sendAsync(const std::shared_ptr<flatbuffers::FlatBufferBuilder> & message);

        bool start(std::optional<const std::function<void(const Message*)>> messageHandler = std::nullopt);
        void stop();
};

struct CommBuilder {
  std::shared_ptr<flatbuffers::FlatBufferBuilder> builder = std::make_shared<flatbuffers::FlatBufferBuilder>(100);
  std::vector<uint8_t> messageTypes;
  std::vector<flatbuffers::Offset<void>> messages;
  bool ack = false;
};

class CommCenter final {
    private:
        std::queue<const Message*> messages;
        std::mutex messageQueueMutex;

    public:
        CommCenter(const CommCenter&) = delete;
        CommCenter& operator=(const CommCenter &) = delete;
        CommCenter(CommCenter &&) = delete;
        CommCenter & operator=(CommCenter) = delete;
        CommCenter() {};

        static std::shared_ptr<flatbuffers::FlatBufferBuilder> createAckMessage();
        static std::shared_ptr<flatbuffers::FlatBufferBuilder> createMessage(CommBuilder & builder);

        static inline const flatbuffers::Offset<ObjectProperties> createObjectProperties(CommBuilder & builder, const Vec3 location, const Vec3 rotation, const float scale);
        static inline const flatbuffers::Offset<CreateObject> createSphereObject(CommBuilder & builder, const flatbuffers::Offset<ObjectProperties> & properties, const float radius);



        static void addCreateSphereObject(CommBuilder & builder, const Vec3 location, const Vec3 rotation, const float scale, const float radius);

        void queueMessages(const Message * message);
        const Message * getNextMessage();

};

#endif

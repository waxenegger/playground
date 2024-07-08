#ifndef SRC_INCLUDES_COMMUNICATION_INCL_H_
#define SRC_INCLUDES_COMMUNICATION_INCL_H_

#include "logging.h"
#include <zmq.h>

#include <chrono>
#include <thread>
#include <functional>
#include <future>
#include <random>

class Communication {
    private:
        static std::default_random_engine default_random_engine;
        static std::uniform_int_distribution<int> distribution;

    protected:
        bool running = false;

        std::string udpAddress;
        std::string tcpAddress;

    public:
        Communication(const Communication&) = delete;
        Communication& operator=(const Communication &) = delete;
        Communication(Communication &&) = delete;
        Communication & operator=(Communication) = delete;

        Communication(const std::string ip = "127.0.0.1", const uint16_t udpPort = 3000, const uint16_t tcpPort = 3001);

        virtual bool start() = 0;
        virtual void stop() = 0;

        static void sleepInMillis(const uint32_t millis);
        static uint64_t getTimeInMillis();
        static uint32_t getRandomUint32();

        virtual ~Communication() {};
};

class CommClient : public Communication {
    private:
        std::vector<std::future<void>> pendingFutures;
        void addAsyncTask(std::future<void> & future);

    public:
        CommClient(const CommClient&) = delete;
        CommClient& operator=(const CommClient &) = delete;
        CommClient(CommClient &&) = delete;
        CommClient & operator=(CommClient) = delete;

        CommClient(const std::string ip = "127.0.0.1", const uint16_t udpPort = 3000, const uint16_t tcpPort = 3001) : Communication(ip, udpPort, tcpPort) {};

        std::string sendBlocking(const std::string message, const bool waitForResponse = true);
        void sendAsync(const std::string message, const bool waitForRespone = true, std::function<void (const std::string)> callback = [](const std::string) {});

        bool start();
        void stop();
};

class CommServer : public Communication {
    private:
        void * udpContext = nullptr;
        void * udpRadio = nullptr;

        void * tcpContext = nullptr;
        void * tcpPub = nullptr;

        bool startUdp();
        bool startTcp();

    public:
        CommServer(const CommServer&) = delete;
        CommServer& operator=(const CommServer &) = delete;
        CommServer(CommServer &&) = delete;
        CommServer & operator=(CommServer) = delete;

        CommServer(const std::string ip = "127.0.0.1", const uint16_t udpPort = 3000, const uint16_t tcpPort = 3001) : Communication(ip, udpPort, tcpPort) {};

        void send(const std::string message);

        bool start();
        void stop();
};

#endif

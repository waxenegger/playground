#ifndef SRC_INCLUDES_COMMUNICATION_INCL_H_
#define SRC_INCLUDES_COMMUNICATION_INCL_H_

#include "logging.h"
#include <zmq.h>

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

    protected:
        bool useInproc = false;
        bool running = false;

        std::string inProcAddress = "inproc://playground";
        std::string udpAddress;
        std::string tcpAddress;

    public:
        Communication(const Communication&) = delete;
        Communication& operator=(const Communication &) = delete;
        Communication(Communication &&) = delete;
        Communication & operator=(Communication) = delete;

        Communication();
        Communication(const std::string ip, const uint16_t udpPort = 3000, const uint16_t tcpPort = 3001);

        virtual bool start() = 0;
        virtual void stop() = 0;

        static void sleepInMillis(const uint32_t millis);
        static uint64_t getTimeInMillis();
        static uint32_t getRandomUint32();

        virtual ~Communication() {};
};

class CommClient : public Communication {
    private:
        void * inProcContext = nullptr;
        void * inProcSocket = nullptr;

        std::vector<std::future<void>> pendingFutures;
        void addAsyncTask(std::future<void> & future);

    public:
        CommClient(const CommClient&) = delete;
        CommClient& operator=(const CommClient &) = delete;
        CommClient(CommClient &&) = delete;
        CommClient & operator=(CommClient) = delete;

        CommClient(void * inProcContext) : Communication() { this->inProcContext = inProcContext; };
        CommClient(const std::string ip, const uint16_t udpPort = 3000, const uint16_t tcpPort = 3001) : Communication(ip, udpPort, tcpPort) {};

        std::string sendInProcMessage(const std::string & message, const bool waitForResponse = true);
        std::string sendBlocking(const std::string & message, const bool waitForResponse = true);
        void sendAsync(const std::string & message, const bool waitForRespone = true, std::function<void (const std::string)> callback = [](const std::string) {});

        bool start();
        bool startInProcClientSocket();
        void stop();
};

class CommServer : public Communication {
    private:
        void * inProcContext = nullptr;
        void * inProcSocket = nullptr;

        void * udpContext = nullptr;
        void * udpRadio = nullptr;

        void * tcpContext = nullptr;
        void * tcpPub = nullptr;

        bool startUdp();
        bool startTcp();
        bool startInproc();

    public:
        CommServer(const CommServer&) = delete;
        CommServer& operator=(const CommServer &) = delete;
        CommServer(CommServer &&) = delete;
        CommServer & operator=(CommServer) = delete;

        CommServer() : Communication() {};
        CommServer(const std::string ip, const uint16_t udpPort = 3000, const uint16_t tcpPort = 3001) : Communication(ip, udpPort, tcpPort) {};

        void send(const std::string & message);

        void * getInProcContext();

        bool start();
        void stop();
};

#endif

#ifndef SRC_INCLUDES_COMMUNICATION_INCL_H_
#define SRC_INCLUDES_COMMUNICATION_INCL_H_

#include "logging.h"
#include <zmq.h>

bool startUdpRadio(const bool & stop, const std::string ip = "127.0.0.1", const uint16_t port = 3000);

#endif

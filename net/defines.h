//
// Created by xiaohu on 3/21/22.
//

#ifndef PANDA_DEFINES_H
#define PANDA_DEFINES_H
#include <functional>
#include <any>
#include "asio.hpp"
#include "protocol/proto.h"
using Task = std::function<void()>;

class MsgBuffer;
using Predict = std::function<bool(MsgBuffer &buffer)>;

class Connection;
using Handler = std::function< void(std::shared_ptr<Connection>, std::shared_ptr<MsgBuffer>&) >;

using TimeHandler = std::function<void()>;


#endif //PANDA_DEFINES_H

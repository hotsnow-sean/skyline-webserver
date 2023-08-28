#pragma once

#include <netinet/in.h>

#include <functional>
#include <memory>
#include <thread>

#include "event_loop.h"

namespace skyline::core {

class Channel;
class Reactor;
class ReadBuffer;

// TcpServer 管理一个主从反应堆
// 使用者可以根据需求重写 AfterConnect 和 OnRecv 函数
// 这两个函数分别会在新连接建立后、收到消息时被调用
class TcpServer {
public:
    TcpServer(const sockaddr_in& addr, Reactor& reactor);
    virtual ~TcpServer() = default;

    // 用户调用该函数启动服务器监听
    void StartListen();
    void StopListen();

protected:
    // 默认为空函数，由子类自行决定干什么
    virtual void AfterConnect(std::shared_ptr<Channel> ctx);
    virtual void OnRecv(std::shared_ptr<Channel> ctx, ReadBuffer& buf);

private:
    int sock_fd_{-1};
    sockaddr_in addr_{};

    Reactor& reactor_;
};

}  // namespace skyline::core

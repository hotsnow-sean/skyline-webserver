#pragma once

#include <netinet/in.h>

#include <functional>
#include <memory>
#include <thread>

#include "event_loop.h"

namespace skyline::core {

class ReadBuffer;
class Channel;

// TcpServer 管理一个主从反应堆
// 使用者可以根据需求重写 AfterConnect 和 OnRecv 函数
// 这两个函数分别会在新连接建立后、收到消息时被调用
class TcpServer {
public:
    // 子反应堆数默认为 0，即单反应堆
    TcpServer(const sockaddr_in& addr, std::size_t sub_reactor_num = 0);
    virtual ~TcpServer();

    // 用户调用该函数启动服务器
    void Start();
    void Stop();

protected:
    // 默认为空函数，由子类自行决定干什么
    virtual void AfterConnect(std::shared_ptr<Channel> ctx);
    virtual void OnRecv(std::shared_ptr<Channel> ctx, ReadBuffer& buf);

private:
    int sock_fd_;
    EventLoop main_reactor_;
    std::vector<EventLoop> sub_reactors_;
    std::vector<std::thread> sub_threads_;

    // 记录当前应使用的子反应堆号，用于平均分配 fd
    size_t cur_reactor_id_{0};
};

}  // namespace skyline::core

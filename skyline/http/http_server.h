#pragma once

#include "core/tcp_server.h"
#include "servlet.h"

namespace skyline::http {

class HttpSession;

using skyline::core::TcpServer;

class HttpServer : public TcpServer {
public:
    using TcpServer::TcpServer;

    void AfterConnect(std::shared_ptr<core::Channel> ctx) override;

    void OnRecv(std::shared_ptr<core::Channel> ctx,
                core::ReadBuffer& buf) override;

public:
    bool is_keepalive{false};
    ServletDispatch dispatch;

    std::map<int, std::shared_ptr<HttpSession>> http_sessions_;
};

}  // namespace skyline::http

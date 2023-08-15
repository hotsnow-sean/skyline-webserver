#include <arpa/inet.h>

#include <iostream>

#include "core/buffer.h"
#include "core/channel.h"
#include "core/event_loop.h"
#include "core/tcp_server.h"

using namespace skyline::core;

class EchoServer : public TcpServer {
public:
    using TcpServer::TcpServer;

    void AfterConnect(std::shared_ptr<Channel> ctx) override {
        std::cout << ctx->fd() << " connected!\n";
    }
    void OnRecv(std::shared_ptr<Channel> ctx, ReadBuffer& buf) override {
        const auto massage = buf.ReadAll();
        std::cout << ctx->fd() << " recv: " << massage << '\n';
        ctx->SendMassage(massage);
    }
};

int main() {
    EchoServer server(::sockaddr_in{
        .sin_family = AF_INET,
        .sin_port = htons(8888),
        .sin_addr = {.s_addr = htonl(INADDR_ANY)},
    });

    server.Start();
    return 0;
}

#include "tcp_server.h"

#include <arpa/inet.h>
#include <fcntl.h>

#include <cstring>

#include "reactor.h"
#include "socket_context.h"
#include "utils.h"

static constexpr size_t kReadBufferLen = 1024;

namespace skyline::core {

namespace detail {

// 负责创建监听 socket，并启动监听
class Acceptor : public SocketContext {
public:
    using AfterAcceptCallback = std::function<void(int)>;

    Acceptor(EventLoop& loop, const sockaddr_in& addr)
        : SocketContext(loop, socket(AF_INET, SOCK_STREAM, 0),
                        EPOLLIN | EPOLLPRI) {
        if (fd() == -1) {
            SYSTEM_LOG_FATAL << "server socket create fail: "
                             << strerror(errno);
            throw "socket create fail";
        }
        int opt = 1;
        if (::setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) ==
            -1) {
            SYSTEM_LOG_FATAL << "set reuse addr fail: [" << fd() << "] "
                             << strerror(errno);
            throw "set reuse addr fail";
        }
        if (::bind(fd(), (sockaddr*)&addr, sizeof addr) == -1) {
            SYSTEM_LOG_FATAL << "addr bind fail: [" << fd() << "] "
                             << strerror(errno);
            throw "socket addr bind fail";
        }
        if (fcntl(fd(), F_SETFL, fcntl(fd(), F_GETFL) | O_NONBLOCK) == -1) {
            SYSTEM_LOG_FATAL << "set nonblock fail: [" << fd() << "] "
                             << strerror(errno);
            throw "set nonblock fail";
        }
        if (::listen(fd(), SOMAXCONN) == -1) {
            SYSTEM_LOG_FATAL << "listen fail: [" << fd() << "] "
                             << strerror(errno);
            throw "listen fail";
        }
        SYSTEM_LOG_INFO << "server listen in: " << inet_ntoa(addr.sin_addr)
                        << ":" << ::ntohs(addr.sin_port);
    }

    bool HandleReadEvent() override {
        auto clnt_sockfd = ::accept(fd(), NULL, NULL);
        if (clnt_sockfd == -1) {
            if ((errno == EAGAIN || errno == EWOULDBLOCK)) {
                return true;
            }
            return false;
        }
        if (after_accept_) after_accept_(clnt_sockfd);
        return true;
    }

    void Close() override { this->loop_.RemoveSocketContext(this->fd()); }

    // 被动监听的 socket 不需要实现该方法
    void SendMassage(const std::string_view& massage) override {}

    void setAfterAcceptCallback(AfterAcceptCallback fun) noexcept {
        after_accept_ = std::move(fun);
    }

private:
    AfterAcceptCallback after_accept_;
};

// 负责管理一个连接 socket
class Connection : public SocketContext,
                   public std::enable_shared_from_this<Connection> {
public:
    using HandleMassageCallback =
        std::function<void(std::shared_ptr<Channel>, ReadBuffer&)>;

    Connection(EventLoop& loop, int fd)
        : SocketContext(loop, fd, EPOLLIN | EPOLLPRI | EPOLLET) {
        if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1) {
            SYSTEM_LOG_ERROR << "set nonblock fail: [" << fd << "] "
                             << strerror(errno);
            Close();
        }
    }

    bool HandleReadEvent() override {
        char buf[kReadBufferLen];
        ssize_t cnt = 0;
        while (true) {
            auto bytes_read = ::read(fd(), buf, sizeof buf);
            if (bytes_read > 0) {
                if (massage_handler_) read_buffer_.Write(buf, bytes_read);
                cnt += bytes_read;
            } else if (bytes_read == -1 && errno == EINTR) {
                continue;
            } else if (bytes_read == -1 &&
                       (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (massage_handler_) {
                    massage_handler_(shared_from_this(), read_buffer_);
                }
                return true;
            } else if (bytes_read == 0) {
                return false;
            }
        }
        return false;
    }

    void SendMassage(const std::string_view& massage) override {
        loop_.RunInLoop([this, &massage]() {
            auto bytes_write =
                ::write(this->fd(), massage.data(), massage.size());
            if (bytes_write < 0) {
                Close();
            } else if (bytes_write < massage.size()) {
                write_buffer_.WriteAll(massage.substr(bytes_write));
                this->events |= EPOLLOUT;
                loop_.UpdateSocketContext(this->fd(), this->events);
            }
        });
    }

    void Close() override { this->loop_.RemoveSocketContext(this->fd()); }

    void setHandleMassageCallback(HandleMassageCallback fun) noexcept {
        massage_handler_ = std::move(fun);
    }

private:
    HandleMassageCallback massage_handler_;
    Buffer read_buffer_;
};

}  // namespace detail

TcpServer::TcpServer(const sockaddr_in& addr, Reactor& reactor)
    : addr_(addr), reactor_(reactor) {}

void TcpServer::StartListen() {
    auto acceptor =
        std::make_shared<detail::Acceptor>(reactor_.main_reactor, addr_);
    sock_fd_ = acceptor->fd();
    acceptor->setAfterAcceptCallback([this, acceptor](int fd) {
        auto& loop = this->reactor_.NextLoop();
        auto conn = std::make_shared<detail::Connection>(loop, fd);
        conn->setHandleMassageCallback(std::bind(&TcpServer::OnRecv, this,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2));
        this->AfterConnect(conn);
        loop.AddSocketContext(conn);
    });
    reactor_.main_reactor.AddSocketContext(std::move(acceptor));
}

void TcpServer::StopListen() {
    reactor_.main_reactor.RemoveSocketContext(sock_fd_);
}

void TcpServer::AfterConnect(std::shared_ptr<Channel> ctx) {}

void TcpServer::OnRecv(std::shared_ptr<Channel> ctx, ReadBuffer& buf) {}

}  // namespace skyline::core

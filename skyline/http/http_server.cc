#include "http_server.h"

#include <cstring>
#include <sstream>

#include "core/buffer.h"
#include "core/channel.h"
#include "http_session.h"

namespace skyline::http {

// 新连接创建之后，为其创建一个新会话，并添加一个关闭定时器
void HttpServer::AfterConnect(std::shared_ptr<core::Channel> ctx) {
    http_sessions_[ctx->fd()] = std::make_shared<HttpSession>();
    http_sessions_[ctx->fd()]->timer_id =
        ctx->loop().AddTimer(500, [this, ctx](auto) {
            http_sessions_[ctx->fd()].reset();
            ctx->Close();
        });
}

void HttpServer::OnRecv(std::shared_ptr<core::Channel> ctx,
                        core::ReadBuffer& buf) {
    // 拿到对应的会话
    auto cur_session = http_sessions_[ctx->fd()];
    // 解析数据
    cur_session->Parse(buf.ReadAll());
    // 解析错误，移除定时器，销毁会话，关闭连接
    if (cur_session->isError()) {
        if (cur_session->timer_id) {
            ctx->loop().RemoveTimer(*cur_session->timer_id);
        }
        http_sessions_[ctx->fd()].reset();
        ctx->Close();
        return;
    }
    // 获取解析完的请求
    auto req = cur_session->TryGet();
    // 未解析完，直接返回，等待下次消息继续解析
    if (!req) return;
    // 创建 response
    HttpResponse res(req->version, req->close || !is_keepalive);
    // 由路径分发器填充 response
    dispatch.handle(*req.get(), res, ctx);
    // 将 response 转为字符串并发送
    std::stringstream ss;
    ss << res;
    std::string data = ss.str();
    ctx->SendMassage(data);
    // 一次交流结束，移除定时器
    if (cur_session->timer_id) {
        ctx->loop().RemoveTimer(*cur_session->timer_id);
    }
    // 如果是长连接，重新初始化会话，重新添加定时器
    if (is_keepalive && !req->close) {
        http_sessions_[ctx->fd()] = std::make_shared<HttpSession>();
        http_sessions_[ctx->fd()]->timer_id =
            ctx->loop().AddTimer(500, [this, &ctx](auto) {
                this->http_sessions_[ctx->fd()].reset();
                ctx->Close();
            });
    } else {
        // 短连接，直接关闭
        this->http_sessions_[ctx->fd()].reset();
        ctx->Close();
    }
}

}  // namespace skyline::http

#pragma once

#include <memory>

#include "core/timer.h"
#include "http_parser.h"

namespace skyline::http {

// 管理一次 http 请求会话，提供
class HttpSession {
public:
    // 根据传入的新数据，继续解析当前请求
    void Parse(const std::string_view& data);

    // 尝试获取解析完毕的请求，未解析完成时返回空指针
    std::unique_ptr<HttpRequest> TryGet();

    bool isError() { return error_; }

public:
    // 当前会话对应的定时器 ID
    std::optional<core::Timer::timer_id_t> timer_id;

private:
    HttpRequestParser parser_;
    std::string buffer_;  // 存储未解析完毕的数据
    bool error_{false};
    bool ok_{false};
};

}  // namespace skyline::http

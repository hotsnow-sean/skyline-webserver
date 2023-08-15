#include "http_session.h"

#include <sstream>

#include "http_parser.h"

namespace skyline::http {

static constexpr size_t kBufSize = 4 * 1024;

void HttpSession::Parse(const std::string_view& data) {
    if (ok_ || error_) return;
    buffer_ += data;
    if (!parser_.isFinished()) {
        auto nparsed = parser_.execute(buffer_.data(), buffer_.size(), 0);
        buffer_.erase(0, nparsed);
        if (parser_.hasError()) {
            error_ = true;
            return;
        }
    }
    if (parser_.isFinished()) {
        auto v = parser_.data().getHeader("content-length");
        auto body_len = v == nullptr ? 0 : ::atol(v->c_str());
        if (buffer_.size() >= body_len) {
            parser_.data().body.swap(buffer_);
            ok_ = true;
        }
    }
}

std::unique_ptr<HttpRequest> HttpSession::TryGet() {
    if (ok_) {
        // 只允许成功获取一次，之后该会话应该销毁
        ok_ = false;
        error_ = true;
        return std::make_unique<HttpRequest>(std::move(parser_.data()));
    } else {
        return {};
    }
}

}  // namespace skyline::http

#include "buffer.h"

static constexpr size_t kDefaultSize = 1024;

namespace skyline::core {

BaseBuffer::BaseBuffer() { data_.reserve(kDefaultSize); }

std::string ReadBuffer::ReadAll() {
    std::string res = data_.substr(idx_);
    idx_ = 0;
    data_.clear();
    return res;
}

std::string ReadBuffer::Read(size_t n) {
    std::string res = data_.substr(idx_, n);
    idx_ += res.size();
    return res;
}

void WriteBuffer::WriteAll(std::string_view data) {
    const int end = data_.size() + data.size();
    const int len = end - idx_;
    if (end > data_.capacity()) {
        if (len < data_.capacity()) {
            data_.erase(0, idx_);
            idx_ = 0;
        } else {
            data_.reserve(end + 1);
        }
    }
    data_ += data;
}

void WriteBuffer::Write(std::string_view data, size_t n) {
    WriteAll(data.substr(0, n));
}

}  // namespace skyline::core

#pragma once

#include <string>

namespace skyline::core {

class BaseBuffer {
public:
    BaseBuffer();
    virtual ~BaseBuffer() = default;

    size_t size() const noexcept { return data_.size() - idx_; }
    const char* data() const noexcept { return data_.data() + idx_; }

protected:
    std::string data_;
    size_t idx_{0};
};

class ReadBuffer : virtual public BaseBuffer {
public:
    std::string ReadAll();
    std::string Read(size_t n);
};

class WriteBuffer : virtual public BaseBuffer {
public:
    void WriteAll(std::string_view data);
    void Write(std::string_view data, size_t n);
};

class Buffer : public ReadBuffer, public WriteBuffer {};

}  // namespace skyline::core

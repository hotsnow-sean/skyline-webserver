#include "async_log.h"

#include <filesystem>
#include <iomanip>

/**
 * features:
 * C++11:
 *  array, unique_ptr, move, thread, atomic_bool
 *  condition_variable, mutex, unique_lock, lock_guard
 * C++17:
 *  string_view
 **/

namespace skyline::logger {

using namespace std::chrono_literals;

static constexpr size_t kFixedBufferSize = 4000 * 1000;
static constexpr auto kFlushInterval = 3s;

class FixedBuffer {
public:
    FixedBuffer() : _current(_buffer.begin()) {}

    bool write(const std::string_view& data) {
        if (avaliableSpace() < data.size()) return false;
        std::copy(data.begin(), data.end(), _current);
        _current += data.size();
        return true;
    }

    constexpr const char* data() const noexcept { return _buffer.data(); }
    void clear() noexcept { _current = _buffer.begin(); }

    size_t length() const noexcept { return _current - _buffer.begin(); }
    size_t avaliableSpace() const noexcept { return _buffer.end() - _current; }

private:
    std::array<char, kFixedBufferSize> _buffer;
    std::array<char, kFixedBufferSize>::iterator _current;
};

AsyncAppender::AsyncAppender()
    : _current_buffer(std::make_unique<FixedBuffer>()),
      _next_buffer(std::make_unique<FixedBuffer>()) {
    _thread = std::thread([&]() {
        auto tmp_buffer = std::make_unique<FixedBuffer>();
        std::vector<std::unique_ptr<FixedBuffer>> buffers_to_write;
        buffers_to_write.reserve(2);

        while (!_stop) {
            // 临界区，负责前后端buffer的交换
            {
                std::unique_lock<std::mutex> lock(_mutex);
                if (_buffers.empty()) {
                    _cv.wait_for(lock, kFlushInterval);
                }
                // 这步操作结束后保证current和next都可用
                if (_buffers.empty() || _stop) {
                    // 超时没满，但current有数据，则强制写入，并用备用buffer替换
                    if (_current_buffer->length() > 0) {
                        _buffers.push_back(std::move(_current_buffer));
                        _current_buffer.swap(tmp_buffer);
                    }
                } else if (!_next_buffer) {
                    // 有满buffer情况下，很有可能next为空，则用备用buffer替换
                    _next_buffer.swap(tmp_buffer);
                }
                if (_buffers.empty()) continue;
                buffers_to_write.swap(_buffers);
            }

            // 写出buffer
            for (const auto& b : buffers_to_write) {
                append(b->data(), b->length());
            }
            // 清理多余buffer
            if (buffers_to_write.size() > 1) {
                buffers_to_write.resize(1);
            }
            // 恢复备用buffer
            if (!tmp_buffer) tmp_buffer.swap(buffers_to_write.front());
            tmp_buffer->clear();
            buffers_to_write.clear();

            // 持久化
            flush();
        }
    });
}

AsyncAppender::~AsyncAppender() {
    stop();
}

void AsyncAppender::log(const Logger& logger, LogLevel level,
                        const LogEvent& event) {
    std::stringstream ss;
    _formatter->format(ss, logger, level, event);
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_current_buffer->write(ss.view())) {
        _buffers.push_back(std::move(_current_buffer));
        if (_next_buffer) {
            _current_buffer.swap(_next_buffer);
        } else {
            _current_buffer = std::make_unique<FixedBuffer>();
        }
        _current_buffer->write(ss.view());
        _cv.notify_one();
    }
}

void AsyncAppender::stop() {
    if (!_stop) {
        _stop = true;
        _cv.notify_one();
        _thread.join();
    }
}

// ---------------------------- AsyncFileAppender ----------------------------

AsyncFileAppender::AsyncFileAppender(const std::string& filename,
                                     bool thread_safe)
    : _filestream(filename, std::ios::app), _mutex(nullptr) {
    if (thread_safe) _mutex = new std::mutex;
}

AsyncFileAppender::~AsyncFileAppender() {
    stop();
    delete _mutex;
}

void AsyncFileAppender::append(const char* data, size_t length) {
    if (_mutex != nullptr) {
        std::lock_guard<std::mutex> lock(*_mutex);
        _filestream.write(data, length);
    } else {
        _filestream.write(data, length);
    }
}

void AsyncFileAppender::flush() {
    if (_mutex != nullptr) {
        std::lock_guard<std::mutex> lock(*_mutex);
        _filestream.flush();
    } else {
        _filestream.flush();
    }
}

// -------------------------- AsyncRollFileAppender --------------------------

static std::string getLogFilename(std::string basename) {
    auto t = std::time(NULL);
    char timebuf[32];
    std::strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S",
                  std::localtime(&t));
    return basename + timebuf + ".log";
}

AsyncRollFileAppender::AsyncRollFileAppender(std::string basename,
                                             size_t limit_size,
                                             bool thread_safe)
    : _basename(std::move(basename)), _limit_size(limit_size) {
    if (thread_safe) _mutex = new std::mutex;
    _filename = getLogFilename(_basename);
    if (std::filesystem::exists(_filename)) {
        _file_size = std::filesystem::file_size(_filename);
    }
    _filestream.open(_filename, std::ios::app);
}

AsyncRollFileAppender::~AsyncRollFileAppender() {
    stop();
    delete _mutex;
}

void AsyncRollFileAppender::append(const char* data, size_t length) {
    if (_mutex != nullptr) _mutex->lock();
    _filestream.write(data, length);
    _file_size += length;
    rollFile();
    if (_mutex != nullptr) _mutex->unlock();
}

void AsyncRollFileAppender::flush() {
    if (_mutex != nullptr) _mutex->lock();
    _filestream.flush();
    rollFile();
    if (_mutex != nullptr) _mutex->unlock();
}

void AsyncRollFileAppender::rollFile() {
    if (_file_size >= _limit_size) {
        _filename = getLogFilename(_basename);
        _filestream.close();
        _filestream.open(_filename, std::ios::app);
        _file_size = 0;
    }
}

}  // namespace skyline::logger

#include "http_parser.h"

#include <cstring>

#include "core/utils.h"

namespace skyline::http {

static void on_request_method(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = reinterpret_cast<HttpRequestParser *>(data);
    auto m = string2HttpMethod(std::string_view(at, length));
    if (m == HttpMethod::INVALID_METHOD) {
        SYSTEM_LOG_WARN << "invalid http request method "
                        << std::string_view(at, length);
        parser->setError(1000);
    }
    parser->data().method = m;
}
static void on_request_uri(void *data, const char *at, size_t length) {
}
static void on_request_fragment(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = reinterpret_cast<HttpRequestParser *>(data);
    parser->data().fragment = std::string(at, length);
}
static void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = reinterpret_cast<HttpRequestParser *>(data);
    parser->data().path = std::string(at, length);
}
static void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = reinterpret_cast<HttpRequestParser *>(data);
    parser->data().query = std::string(at, length);
}
static void on_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = reinterpret_cast<HttpRequestParser *>(data);
    uint8_t v = 0;
    if (::strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if (::strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        SYSTEM_LOG_WARN << "invalid http request version: "
                        << std::string_view(at, length);
        parser->setError(1001);
        return;
    }
    parser->data().version = v;
}
static void on_request_header_done(void *data, const char *at, size_t length) {
}
static void on_request_http_field(void *data, const char *field, size_t flen,
                                  const char *value, size_t vlen) {
    HttpRequestParser *parser = reinterpret_cast<HttpRequestParser *>(data);
    if (flen == 0) {
        SYSTEM_LOG_WARN << "invalid http request field length == 0";
        parser->setError(1002);
        return;
    }
    if (::strncasecmp(field, "connection", 10) == 0) {
        if (::strncasecmp(value, "close", 5) == 0) {
            parser->data().close = true;
        } else {
            parser->data().close = false;
        }
    }
    parser->data().setHeader(std::string(field, flen),
                             std::string(value, vlen));
}

HttpRequestParser::HttpRequestParser() {
    http_parser_init(&_parser);
    _parser.request_method = on_request_method;
    _parser.request_uri = on_request_uri;
    _parser.fragment = on_request_fragment;
    _parser.request_path = on_request_path;
    _parser.query_string = on_request_query;
    _parser.http_version = on_request_version;
    _parser.header_done = on_request_header_done;
    _parser.http_field = on_request_http_field;
    _parser.data = this;
}

size_t HttpRequestParser::execute(const char *buffer, size_t len, size_t off) {
    if (off > len) {
        SYSTEM_LOG_WARN << "http parse acquire offset <= len";
        setError(1003);
        return _parser.nread;
    }
    auto p = buffer + off;
    auto pe = buffer + len - 1;
    while (*pe != '\n' && pe > p) {
        --pe;
    }
    if (p == pe) return _parser.nread;
    return http_parser_execute(&_parser, buffer, pe + 1 - buffer, off);
}

int HttpRequestParser::isFinished() {
    return http_parser_finish(&_parser);
}

int HttpRequestParser::hasError() {
    return _error || http_parser_has_error(&_parser);
}

static void on_response_reason(void *data, const char *at, size_t length) {
    HttpResponseParser *parser = reinterpret_cast<HttpResponseParser *>(data);
    parser->data().reason = std::string(at, length);
}
static void on_response_status(void *data, const char *at, size_t length) {
    HttpResponseParser *parser = reinterpret_cast<HttpResponseParser *>(data);
    parser->data().status = static_cast<HttpStatus>(::atoi(at));
}
static void on_response_chunk(void *data, const char *at, size_t length) {
}
static void on_response_version(void *data, const char *at, size_t length) {
    HttpResponseParser *parser = reinterpret_cast<HttpResponseParser *>(data);
    uint8_t v = 0;
    if (::strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if (::strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        SYSTEM_LOG_WARN << "invalid http response version: "
                        << std::string_view(at, length);
        parser->setError(1001);
        return;
    }
    parser->data().version = v;
}
static void on_response_header_done(void *data, const char *at, size_t length) {
}
static void on_response_last_chunk(void *data, const char *at, size_t length) {
}
static void on_response_http_field(void *data, const char *field, size_t flen,
                                   const char *value, size_t vlen) {
    HttpResponseParser *parser = reinterpret_cast<HttpResponseParser *>(data);
    if (flen == 0) {
        SYSTEM_LOG_WARN << "invalid http response field length == 0";
        parser->setError(1002);
        return;
    }
    if (::strncasecmp(field, "connection", 10) == 0) {
        if (::strncasecmp(value, "close", 5) == 0) {
            parser->data().close = true;
        } else {
            parser->data().close = false;
        }
    }
    parser->data().setHeader(std::string(field, flen),
                             std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser() {
    httpclient_parser_init(&_parser);
    _parser.reason_phrase = on_response_reason;
    _parser.status_code = on_response_status;
    _parser.chunk_size = on_response_chunk;
    _parser.http_version = on_response_version;
    _parser.header_done = on_response_header_done;
    _parser.last_chunk = on_response_last_chunk;
    _parser.http_field = on_response_http_field;
    _parser.data = this;
}

size_t HttpResponseParser::execute(const char *buffer, size_t len, size_t off) {
    httpclient_parser_init(&_parser);
    return httpclient_parser_execute(&_parser, buffer, len, off);
}

int HttpResponseParser::isFinished() {
    return httpclient_parser_finish(&_parser);
}

int HttpResponseParser::hasError() {
    return _error || httpclient_parser_has_error(&_parser);
}

int HttpResponseParser::chunkLength() {
    return _parser.chunked ? _parser.content_len : 0;
}

}  // namespace skyline::http

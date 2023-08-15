#pragma once

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace skyline::http {

class HttpRequestParser {
public:
    HttpRequestParser();

    size_t execute(const char* buffer, size_t len, size_t off);
    int isFinished();
    int hasError();

    HttpRequest& data() { return _data; }
    void setError(int e) { _error = e; }

private:
    http_parser _parser{};
    HttpRequest _data;
    int _error{};
};

class HttpResponseParser {
public:
    HttpResponseParser();

    size_t execute(const char* buffer, size_t len, size_t off);
    int isFinished();
    int hasError();

    HttpResponse& data() { return _data; }
    void setError(int e) { _error = e; }

    int chunkLength();

private:
    httpclient_parser _parser{};
    HttpResponse _data;
    int _error{0};
};

}  // namespace skyline::http

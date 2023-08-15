#include "http.h"

#include <cstring>
#include <ostream>
#include <unordered_map>

namespace skyline::http {

HttpMethod string2HttpMethod(const std::string_view &s) {
  using namespace std::string_view_literals;
  static const std::unordered_map<std::string_view, HttpMethod> mm{
#define XX(num, name, string) {#string##sv, HttpMethod::HTTP_##name},
      HTTP_METHOD_MAP(XX)
#undef XX
  };
  auto it = mm.find(s);
  if (it == mm.end())
    return HttpMethod::INVALID_METHOD;
  return it->second;
}

const char *httpMethod2String(HttpMethod m) {
  switch (m) {
#define XX(num, name, string)                                                  \
  case HttpMethod::HTTP_##name:                                                \
    return #string;
    HTTP_METHOD_MAP(XX)
#undef XX
  case HttpMethod::INVALID_METHOD:
    break;
  }
  return nullptr;
}

const char *httpStatus2String(HttpStatus s) {
  switch (s) {
#define XX(code, name, msg)                                                    \
  case HttpStatus::HTTP_STATUS_##name:                                         \
    return #msg;
    HTTP_STATUS_MAP(XX)
#undef XX
  }
  return nullptr;
}

namespace detail {

bool CaseInsensitiveLess::operator()(const std::string &lhd,
                                     const std::string &rhd) const {
  return ::strcasecmp(lhd.c_str(), rhd.c_str()) < 0;
}

} // namespace detail

HttpRequest::HttpRequest(unsigned char version, bool close)
    : version(version), close(close) {}

const std::string *HttpRequest::getHeader(const std::string &key) const {
  auto it = _headers.find(key);
  if (it == _headers.end())
    return nullptr;
  return &it->second;
}

const std::string *HttpRequest::getParam(const std::string &key) const {
  auto it = _params.find(key);
  if (it == _params.end())
    return nullptr;
  return &it->second;
}

const std::string *HttpRequest::getCookie(const std::string &key) const {
  auto it = _cookies.find(key);
  if (it == _cookies.end())
    return nullptr;
  return &it->second;
}

void HttpRequest::setHeader(const std::string &key, std::string value) {
  _headers[key].swap(value);
}

void HttpRequest::setParam(const std::string &key, std::string value) {
  _params[key].swap(value);
}

void HttpRequest::setCookie(const std::string &key, std::string value) {
  _cookies[key].swap(value);
}

void HttpRequest::delHeader(const std::string &key) { _headers.erase(key); }

void HttpRequest::delParam(const std::string &key) { _params.erase(key); }

void HttpRequest::delCookie(const std::string &key) { _cookies.erase(key); }

bool HttpRequest::hasHeader(const std::string &key) const {
  return _headers.contains(key);
}

bool HttpRequest::hasParam(const std::string &key) const {
  return _params.contains(key);
}

bool HttpRequest::hasCookie(const std::string &key) const {
  return _cookies.contains(key);
}

std::ostream &operator<<(std::ostream &os, const HttpRequest &req) {
  os << httpMethod2String(req.method) << " " << req.path;
  if (!req.query.empty()) {
    os << "?" << req.query;
  }
  if (!req.fragment.empty()) {
    os << "#" << req.fragment;
  }
  os << " HTTP/" << (req.version >> 4) << "." << (req.version & 0x0F) << "\r\n";
  os << "connection: " << (req.close ? "close" : "keep-alive") << "\r\n";
  for (auto &[key, value] : req._headers) {
    if (::strcasecmp(key.c_str(), "connection") == 0)
      continue;
    os << key << ": " << value << "\r\n";
  }
  if (!req.body.empty()) {
    os << "content-length: " << req.body.size() << "\r\n\r\n" << req.body;
  } else {
    os << "\r\n";
  }
  return os;
}

HttpResponse::HttpResponse(uint8_t version, bool close)
    : version(version), close(close) {}

const std::string *HttpResponse::getHeader(const std::string &key) const {
  auto it = _headers.find(key);
  if (it == _headers.end())
    return nullptr;
  return &it->second;
}

void HttpResponse::setHeader(const std::string &key, std::string value) {
  _headers[key].swap(value);
}

void HttpResponse::delHeader(const std::string &key) { _headers.erase(key); }

std::ostream &operator<<(std::ostream &os, const HttpResponse &res) {
  os << "HTTP/" << (res.version >> 4) << "." << (res.version & 0x0F) << " "
     << static_cast<uint32_t>(res.status) << " ";
  if (res.reason.empty()) {
    os << httpStatus2String(res.status);
  } else {
    os << res.reason;
  }
  os << "\r\n";
  for (auto &[key, value] : res._headers) {
    if (::strcasecmp(key.c_str(), "connection") == 0)
      continue;
    os << key << ": " << value << "\r\n";
  }
  os << "connection: " << (res.close ? "close" : "keep-alive") << "\r\n";
  if (!res.body.empty()) {
    os << "content-length: " << res.body.size() << "\r\n\r\n" << res.body;
  } else {
    os << "\r\n";
  }
  return os;
}

} // namespace skyline::http

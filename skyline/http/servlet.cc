#include "servlet.h"

#include <fnmatch.h>

namespace skyline::http {

Servlet::Servlet(std::string name) : name(std::move(name)) {}

FunctionServlet::FunctionServlet(Callback cb)
    : Servlet("FunctionServlet"), _cb(std::move(cb)) {}

int FunctionServlet::handle(const HttpRequest& request, HttpResponse& response,
                            std::shared_ptr<core::Channel> session) {
    return _cb(request, response, session);
}

NotFoundServlet::NotFoundServlet() : Servlet("NotFoundServlet") {}

int NotFoundServlet::handle(const HttpRequest& request, HttpResponse& response,
                            std::shared_ptr<core::Channel> session) {
    static const std::string res_body =
        "<html><head><title>404 Not Found</title></head><body><center><h1>404 "
        "Not "
        "Found</h1></center><hr/><center>skyline/1.0.0</center></body></html>";
    response.status = HttpStatus::HTTP_STATUS_NOT_FOUND;
    response.setHeader("Server", "skyline/1.0.0");
    response.setHeader("Content-Type", "text/html");
    response.body = res_body;
    return 0;
}

ServletDispatch::ServletDispatch()
    : Servlet("ServletDispatch"),
      _default(std::make_unique<NotFoundServlet>()) {}

int ServletDispatch::handle(const HttpRequest& request, HttpResponse& response,
                            std::shared_ptr<core::Channel> session) {
    return getMatchedServlet(request.path)->handle(request, response, session);
}

void ServletDispatch::addServlet(const std::string& uri,
                                 std::unique_ptr<Servlet> slt) {
    if (slt) {
        std::lock_guard lock(_mtx);
        _datas[uri].swap(slt);
    }
}

void ServletDispatch::addServlet(const std::string& uri,
                                 FunctionServlet::Callback cb) {
    if (cb) {
        addServlet(uri, std::make_unique<FunctionServlet>(std::move(cb)));
    }
}

void ServletDispatch::addGlobServlet(const std::string& uri,
                                     std::unique_ptr<Servlet> slt) {
    if (!slt) return;
    std::lock_guard lock(_mtx);
    auto it = std::find_if(_globs.begin(), _globs.end(),
                           [&](auto& p) { return p.first == uri; });
    if (it != _globs.end()) _globs.erase(it);
    _globs.emplace_back(uri, std::move(slt));
}

void ServletDispatch::addGlobServlet(const std::string& uri,
                                     FunctionServlet::Callback cb) {
    if (cb) {
        addGlobServlet(uri, std::make_unique<FunctionServlet>(std::move(cb)));
    }
}

void ServletDispatch::delServlet(const std::string& uri) {
    std::lock_guard lock(_mtx);
    _datas.erase(uri);
}

void ServletDispatch::delGlobServlet(const std::string& uri) {
    std::lock_guard lock(_mtx);
    auto it = std::find_if(_globs.begin(), _globs.end(),
                           [&](auto& p) { return p.first == uri; });
    if (it != _globs.end()) _globs.erase(it);
}

void ServletDispatch::setDefault(std::unique_ptr<Servlet> slt) {
    if (slt) {
        std::lock_guard lock(_mtx);
        _default.swap(slt);
    }
}

std::unique_ptr<Servlet>& ServletDispatch::getMatchedServlet(
    const std::string& uri) {
    std::shared_lock lock(_mtx);
    if (auto it = _datas.find(uri); it != _datas.end()) {
        return it->second;
    }
    auto it = std::find_if(_globs.begin(), _globs.end(), [&](auto& p) {
        return ::fnmatch(p.first.c_str(), uri.c_str(), 0) == 0;
    });
    if (it != _globs.end()) {
        return it->second;
    }
    return _default;
}

}  // namespace skyline::http

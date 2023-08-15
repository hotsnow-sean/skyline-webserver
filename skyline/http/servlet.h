#pragma once

#include <functional>
#include <memory>
#include <shared_mutex>

#include "http.h"

namespace skyline {

namespace core {

class Channel;

}

namespace http {

class Servlet {
public:
    Servlet(std::string name);

    virtual ~Servlet() = default;

    virtual int handle(const HttpRequest& request, HttpResponse& response,
                       std::shared_ptr<core::Channel> session) = 0;

public:
    const std::string name;
};

class FunctionServlet : public Servlet {
public:
    using Callback =
        std::function<int(const HttpRequest& request, HttpResponse& response,
                          std::shared_ptr<core::Channel> session)>;

    FunctionServlet(Callback cb);

    int handle(const HttpRequest& request, HttpResponse& response,
               std::shared_ptr<core::Channel> session) override;

private:
    Callback _cb;
};

class NotFoundServlet : public Servlet {
public:
    NotFoundServlet();

    int handle(const HttpRequest& request, HttpResponse& response,
               std::shared_ptr<core::Channel> session) override;
};

class ServletDispatch : public Servlet {
public:
    ServletDispatch();

    int handle(const HttpRequest& request, HttpResponse& response,
               std::shared_ptr<core::Channel> session) override;

    void addServlet(const std::string& uri, std::unique_ptr<Servlet> slt);
    void addServlet(const std::string& uri, FunctionServlet::Callback cb);
    void addGlobServlet(const std::string& uri, std::unique_ptr<Servlet> slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::Callback cb);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    void setDefault(std::unique_ptr<Servlet> slt);

    std::unique_ptr<Servlet>& getMatchedServlet(const std::string& uri);

private:
    // uri(/skyline/xxx) -> servlet
    std::unordered_map<std::string, std::unique_ptr<Servlet>> _datas;
    // uti(/skyline/*) -> servlet
    std::vector<std::pair<std::string, std::unique_ptr<Servlet>>> _globs;
    // default
    std::unique_ptr<Servlet> _default;
    std::shared_mutex _mtx;
};

}  // namespace http

}  // namespace skyline

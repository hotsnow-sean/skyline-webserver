#include <signal.h>

#include "core/event_loop.h"
#include "core/utils.h"
#include "http/http_server.h"
#include "logger/log.h"

using namespace skyline::core;
using namespace skyline::http;

HttpServer* server_ptr{};
auto& kLogger = skyline::logger::getRootLogger();

void sigint_handler(int sig) {
    if (sig == SIGINT) {
        SKYLINE_LOG_INFO(kLogger) << "stop server...";
        if (server_ptr != nullptr) server_ptr->Stop();
    }
}

int main() {
    std::stringstream ss;
    signal(SIGINT, sigint_handler);
    HttpServer server(
        ::sockaddr_in{
            .sin_family = AF_INET,
            .sin_port = htons(8889),
            .sin_addr = {.s_addr = htonl(INADDR_ANY)},
        },
        4);
    server_ptr = &server;
    server.dispatch.addServlet(
        "/skyline/xx",
        [&ss](const HttpRequest& req, HttpResponse& res, auto session) {
            ss.str("");
            ss << req;
            res.body = ss.str();
            return 0;
        });
    server.dispatch.addGlobServlet(
        "/skyline/*",
        [&ss](const HttpRequest& req, HttpResponse& res, auto session) {
            ss.str("");
            ss << req;
            res.body = "Glob\r\n" + ss.str();
            return 0;
        });
    getSystemLogger().level = skyline::logger::LogLevel::ERROR;
    server.Start();
    return 0;
}

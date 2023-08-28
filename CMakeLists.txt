cmake_minimum_required(VERSION 3.10)
project(skyline_thread)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_CORE_SRC
  skyline/logger/log.cc
  skyline/logger/async_log.cc
  skyline/core/timer.cc
  skyline/core/buffer.cc
  skyline/core/channel.cc
  skyline/core/socket_context.cc
  skyline/core/event_loop.cc
  skyline/core/reactor.cc
  skyline/core/tcp_server.cc
  skyline/core/utils.cc
)

set(LIB_HTTP_SRC
  skyline/http/http_parser.cc
  skyline/http/http.cc
  skyline/http/http11_parser.rl.cc
  skyline/http/httpclient_parser.rl.cc
  skyline/http/servlet.cc
  skyline/http/http_session.cc
  skyline/http/http_server.cc
)

add_library(skyline_core SHARED ${LIB_CORE_SRC})
target_include_directories(skyline_core INTERFACE ${PROJECT_SOURCE_DIR}/skyline)

add_library(skyline_http SHARED ${LIB_HTTP_SRC})
target_link_libraries(skyline_http PUBLIC skyline_core)
target_include_directories(skyline_http INTERFACE ${PROJECT_SOURCE_DIR}/skyline)

add_subdirectory(${PROJECT_SOURCE_DIR}/examples)

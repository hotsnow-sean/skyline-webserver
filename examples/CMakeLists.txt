add_executable(logger_test logger_test.cc)
target_link_libraries(logger_test skyline_core)

add_executable(echo_test echo_test.cc)
target_link_libraries(echo_test skyline_core)

add_executable(http_test http_test.cc)
target_link_libraries(http_test skyline_http)

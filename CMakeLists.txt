cmake_minimum_required(VERSION 3.10)
project(sylar)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "-std=c++11 -O0 -ggdb -Wall -Wno-builtin-macro-redefined ")

include_directories(.)

set(LIB_SRC
    sylar/fiber.cpp
    sylar/iomanager.cpp
    sylar/log.cpp
    sylar/mutex.cpp
    sylar/scheduler.cpp
    sylar/util.cpp
    sylar/thread.cpp
    sylar/timer.cpp
    sylar/hook.cpp
    )

add_library(sylar SHARED ${LIB_SRC})
redefine_file_macro(sylar)

find_library(PTHREAD pthread)

set(LIBS
    sylar
    ${PTHREAD})

add_executable(test_log tests/test_log.cpp)
redefine_file_macro(test_log)
add_dependencies(test_log sylar)
target_link_libraries(test_log ${LIBS})

add_executable(test_thread tests/test_thread.cpp)
redefine_file_macro(test_thread)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread ${LIBS})

add_executable(test_fiber tests/test_fiber.cpp)
redefine_file_macro(test_fiber)
add_dependencies(test_fiber sylar)
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cpp)
redefine_file_macro(test_scheduler)
add_dependencies(test_scheduler sylar)
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cpp)
redefine_file_macro(test_iomanager)
add_dependencies(test_iomanager sylar)
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_timer tests/test_timer.cpp)
redefine_file_macro(test_timer)
add_dependencies(test_timer sylar)
target_link_libraries(test_timer ${LIBS})

add_executable(test_hook tests/test_hook.cpp)
redefine_file_macro(test_hook)
add_dependencies(test_hook sylar)
target_link_libraries(test_hook ${LIBS})

add_executable(tcp_server tests/tcp_server.cpp)
redefine_file_macro(tcp_server)
add_dependencies(tcp_server sylar)
target_link_libraries(tcp_server ${LIBS})


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

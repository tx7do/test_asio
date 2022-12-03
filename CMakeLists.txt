# 设置cmake的最低版本
cmake_minimum_required(VERSION 3.16)

# 设置工程名称 和版本号
project(test_asio LANGUAGES CXX VERSION 1.0)

# 指定C++版本
SET(CMAKE_CXX_STANDARD 20)

# 设置指定的C++编译器版本是必须的，如果不设置，或者为OFF，则指定版本不可用时，会使用上一版本。
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.71.0 REQUIRED COMPONENTS system)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()


add_subdirectory(chat/server)
add_subdirectory(chat/client)

add_subdirectory(echo/server)
add_subdirectory(echo/client)

#add_subdirectory(echo_ts/server)
#add_subdirectory(echo_ts/client)

add_subdirectory(daytime/server)
add_subdirectory(daytime/client)

add_subdirectory(timer)
add_subdirectory(tcp_server)
add_subdirectory(hello_world)
add_subdirectory(pplx)
add_subdirectory(strand)

# 设置cmake的最低版本
cmake_minimum_required(VERSION 3.16)

# 设置工程名称 和版本号
project(test_asio LANGUAGES CXX VERSION 1.0)

# 打印版本信息
message(STATUS "system: ${CMAKE_SYSTEM}")
message(STATUS "build_type: ${CMAKE_BUILD_TYPE}")
message(STATUS "paths:")
message(STATUS " - ${PROJECT_SOURCE_DIR}")
message(STATUS " - ${CMAKE_INSTALL_PREFIX}")
message(STATUS " - ${CMAKE_SOURCE_DIR}")
message(STATUS " - ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS)
message(STATUS "compiler:")
message(STATUS " - ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " - ${CMAKE_CXX_COMPILER}")
message(STATUS)
message(STATUS "cmake:")
message(STATUS " - ${CMAKE_VERSION}")
message(STATUS " - ${CMAKE_COMMAND}")
message(STATUS " - ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS " - ${CMAKE_GENERATOR}")
message(STATUS " - ${CMAKE_BUILD_TOOL}")
message(STATUS)

include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
# include(CheckCXXSymbolExists)
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(WIN32)
        check_cxx_compiler_flag("/std:c++latest"            support_latest)
        check_cxx_compiler_flag("/clang:-fcoroutines-ts"    support_coroutine)
        check_include_file_cxx("experimental/coroutine" has_coroutine_ts
                "/std:c++latest"
                )
    else()
        check_cxx_compiler_flag("-std=c++2a"          support_latest)
        check_cxx_compiler_flag("-fcoroutines-ts"     support_coroutine)
        check_include_file_cxx("experimental/coroutine" has_coroutine_ts
                "-std=c++2a"
                )
    endif()

elseif(MSVC)
    #
    # Notice that `/std:c++latest` and `/await` is exclusive to each other.
    # With MSVC, we have to distinguish Coroutines TS & C++ 20 Coroutines
    #
    check_cxx_compiler_flag("/std:c++latest"    support_latest)
    check_cxx_compiler_flag("/await"            support_coroutine)
    check_include_file_cxx("coroutine"  has_coroutine
            "/std:c++latest"
            )
    if(NOT has_coroutine)
        message(STATUS "Try <expeirmental/coroutine> (Coroutines TS) instead of <coroutine> ...")
        check_include_file_cxx("experimental/coroutine" has_coroutine_ts
                "/std:c++17"
                )
    endif()
    # has coroutine headers?
    if(NOT has_coroutine AND NOT has_coroutine_ts)
        message(FATAL_ERROR "There are no headers for C++ Coroutines")
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    #
    # expect GCC 10 or later
    #
    check_cxx_compiler_flag("-std=gnu++20"        support_latest)
    check_cxx_compiler_flag("-fcoroutines"        support_coroutine)
    check_include_file_cxx("coroutine" has_coroutine
            "-std=gnu++20 -fcoroutines"
            )
    if(APPLE)
        # -isysroot "/usr/local/Cellar/gcc/${CMAKE_CXX_COMPILER_VERSION}/include/c++/${CMAKE_CXX_COMPILER_VERSION}"
        # -isysroot "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include"
        # -isysroot "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1"
    endif()
    set(try_compile_flags "-fcoroutines")
endif()
# support compiler options for coroutine?
if(NOT support_coroutine)
#    message(FATAL_ERROR "The compiler doesn't support C++ Coroutines")
endif()


# 指定C++版本
set(CMAKE_CXX_STANDARD 20)

# 设置指定的C++编译器版本是必须的，如果不设置，或者为OFF，则指定版本不可用时，会使用上一版本。
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O2 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")

find_package(Boost 1.71.0 REQUIRED COMPONENTS system)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_subdirectory(chat/server)
add_subdirectory(chat/client)

add_subdirectory(echo/server)
add_subdirectory(echo/client)

add_subdirectory(echo_ts/server)
add_subdirectory(echo_ts/client)

add_subdirectory(daytime/server)
add_subdirectory(daytime/client)

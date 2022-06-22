project(asio_timer)

aux_source_directory(. DIR_SRCS)

include_directories(../../)

# 增加生成可执行文件
add_executable(${PROJECT_NAME} ${DIR_SRCS})

target_link_libraries(${PROJECT_NAME} pthread ${Boost_LIBRARIES})
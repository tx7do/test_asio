aux_source_directory(. DIR_SRCS)

include_directories(../../)

# 增加生成可执行文件
add_executable(echo_ts_server ${DIR_SRCS})

target_link_libraries(echo_ts_server pthread ${Boost_LIBRARIES})
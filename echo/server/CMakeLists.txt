aux_source_directory(. DIR_SRCS)

add_executable(echo_server ${DIR_SRCS})

include_directories(../../)

target_link_libraries(echo_server pthread ${Boost_LIBRARIES})
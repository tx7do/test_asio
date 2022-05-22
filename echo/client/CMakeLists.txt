aux_source_directory(. DIR_SRCS)

add_executable(echo_client ${DIR_SRCS})

include_directories(../../)

target_link_libraries(echo_client pthread ${Boost_LIBRARIES})
aux_source_directory(. DIR_SRCS)

add_executable(daytime_server ${DIR_SRCS})

include_directories(../../)

target_link_libraries(daytime_server pthread ${Boost_LIBRARIES})
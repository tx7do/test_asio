aux_source_directory(. DIR_SRCS)

add_executable(daytime_client ${DIR_SRCS})

include_directories(../../)

target_link_libraries(daytime_client pthread ${Boost_LIBRARIES})
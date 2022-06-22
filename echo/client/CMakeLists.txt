project(echo_client)

aux_source_directory(. DIR_SRCS)

add_executable(${PROJECT_NAME} ${DIR_SRCS})

include_directories(../../)

target_link_libraries(${PROJECT_NAME} pthread ${Boost_LIBRARIES})
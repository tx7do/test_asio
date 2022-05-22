aux_source_directory(. DIR_SRCS)

add_executable(chat_server ${DIR_SRCS} ../public/chat_message.h)

include_directories(../../)

target_link_libraries(chat_server pthread ${Boost_LIBRARIES})
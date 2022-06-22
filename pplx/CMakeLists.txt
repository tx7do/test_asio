project(pplx)

aux_source_directory(. DIR_SRCS)

# 增加生成可执行文件
add_executable(${PROJECT_NAME} ${DIR_SRCS})

target_link_libraries(
        ${PROJECT_NAME}
        ${Boost_LIBRARIES}
        boost_thread
        boost_date_time
        pthread
)

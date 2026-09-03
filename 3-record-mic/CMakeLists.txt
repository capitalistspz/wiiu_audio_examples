find_package(PkgConfig REQUIRED)
pkg_search_module(libopus REQUIRED IMPORTED_TARGET opus)

add_executable(record-mic
        main.c
)
target_link_libraries(record-mic PRIVATE PkgConfig::libopus)
target_link_libraries(record-mic PRIVATE simple_wav)


wut_create_rpx(record-mic)
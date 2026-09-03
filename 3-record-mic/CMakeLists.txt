
add_executable(record-mic
    main.c
)
target_link_libraries(record-mic PRIVATE simple_wav)
wut_create_rpx(record-mic)
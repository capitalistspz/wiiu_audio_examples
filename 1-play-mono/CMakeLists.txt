
add_executable(play-mono
    main.c)

target_link_libraries(play-mono stb_vorbis)
wut_create_rpx(play-mono)

add_executable(play-stereo
    main.c)

target_link_libraries(play-stereo PRIVATE stb_vorbis)
wut_create_rpx(play-stereo)
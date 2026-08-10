
add_executable(stereo
    main.c)

target_link_libraries(stereo stb_vorbis)
wut_create_rpx(stereo)
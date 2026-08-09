
add_executable(mono
    main.c)

target_link_libraries(mono stb_vorbis)
wut_create_rpx(mono)
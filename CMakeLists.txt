cmake_minimum_required(VERSION 4.3)
project(wiiu_audio_examples C)

set(CMAKE_C_STANDARD 23)
add_subdirectory(third-party/stb_vorbis EXCLUDE_FROM_ALL)
add_subdirectory(third-party/simple_wav EXCLUDE_FROM_ALL)
add_subdirectory("1-play-mono")
add_subdirectory("2-play-stereo")
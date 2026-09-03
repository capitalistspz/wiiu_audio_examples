project(wut_extra
    LANGUAGES C
    DESCRIPTION "An header library with function signatures that WUT should have but doesn't")
add_library(wut_extra INTERFACE
    wut_extra/multivoice.h
)

target_include_directories(wut_extra INTERFACE .)
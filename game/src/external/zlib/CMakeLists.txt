cmake_minimum_required (VERSION 3.3)
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
add_library(zlib
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzio.c
    infback.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c
)
target_include_directories(zlib INTERFACE .)

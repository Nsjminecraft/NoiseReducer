cmake_minimum_required(VERSION 3.22)
project(anc)

# Oboe via prebuilt — ships with NDK module
find_package(oboe REQUIRED CONFIG)

add_library(anc SHARED
    jni_bridge.cpp
)

target_include_directories(anc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(anc android log oboe::oboe)
target_compile_options(anc PRIVATE -O2 -Wall)

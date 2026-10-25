cmake_minimum_required(VERSION 3.16)
project(noise_reduction)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PortAudio
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

add_executable(noise_reduction src/main.cpp)

target_include_directories(noise_reduction PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
target_link_libraries(noise_reduction ${PORTAUDIO_LIBRARIES})
target_compile_options(noise_reduction PRIVATE -O2 -Wall)

install(TARGETS noise_reduction DESTINATION bin)

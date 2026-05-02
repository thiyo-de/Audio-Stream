cmake_minimum_required(VERSION 3.22.1)
project("audiostream_jni")

# Import the shared core library
# Relative path from: android_app/app/src/main/cpp
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../shared_core shared_core)

add_library(audiostream_jni SHARED
    jni_bridge.cpp
)

target_link_libraries(audiostream_jni
    shared_core
    log
)

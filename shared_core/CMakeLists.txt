include(FetchContent)

# Automatically download and compile Opus from source
FetchContent_Declare(
    opus
    GIT_REPOSITORY https://github.com/xiph/opus.git
    GIT_TAG v1.4
)

set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(opus)

add_library(shared_core STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/TcpUsbTransport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/UdpTransport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OpusCodec.cpp
)

# Expose the include and transport directories to any target linking this
target_include_directories(shared_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/transport
)

target_link_libraries(shared_core PUBLIC opus)

if(WIN32)
    target_link_libraries(shared_core PUBLIC ws2_32)
endif()

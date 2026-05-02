add_executable(AudioStreamHost
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WasapiRender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WasapiLoopback.cpp
)

# Link the Windows executable against our cross-platform shared_core library
target_link_libraries(AudioStreamHost PRIVATE shared_core)

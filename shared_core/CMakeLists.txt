add_library(shared_core INTERFACE)

# Expose the include and transport directories to any target linking this
target_include_directories(shared_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/transport
)

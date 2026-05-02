add_library(shared_core STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/TcpUsbTransport.cpp
)

# Expose the include and transport directories to any target linking this
target_include_directories(shared_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/transport
)

if(WIN32)
    target_link_libraries(shared_core PUBLIC ws2_32)
endif()

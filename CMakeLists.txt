cmake_minimum_required(VERSION 3.14)
project(AudioStream VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the shared core logic as a subdirectory
add_subdirectory(shared_core)

# Future:
# add_subdirectory(windows_app)

cmake_minimum_required(VERSION 2.8.12)
project(OSVRMouseTracker)

# Package Dependencies
find_package(osvr)

# Source Files
add_executable(OSVRMouseTracker ../MouseTracker.cpp)

# Libraries
target_link_libraries(OSVRMouseTracker osvr::osvrClientKitCpp)

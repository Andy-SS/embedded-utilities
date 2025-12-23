cmake_minimum_required(VERSION 3.16)

project(EmbeddedUtilities VERSION 0.1.0)

# Set C standard to C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add subdirectories for each utility
add_subdirectory(eLog)
add_subdirectory(ring)
add_subdirectory(bit)

# Optional: Create library target that includes all utilities
add_library(EmbeddedUtilities INTERFACE)
target_include_directories(EmbeddedUtilities INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/eLog
    ${CMAKE_CURRENT_SOURCE_DIR}/ring
    ${CMAKE_CURRENT_SOURCE_DIR}/bit
)

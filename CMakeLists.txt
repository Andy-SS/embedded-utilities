cmake_minimum_required(VERSION 3.16)

project(EmbeddedUtilities VERSION 0.2.0)

# Set C standard to C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Create common utilities library (includes common.c)
add_library(common STATIC common.c)
target_include_directories(common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add subdirectories for each utility
add_subdirectory(eLog)
add_subdirectory(ring)
add_subdirectory(bit)

# Link common.c to eLog and ring
target_link_libraries(eLog PUBLIC common)
target_link_libraries(ring PUBLIC common)

# Optional: Create library target that includes all utilities
add_library(EmbeddedUtilities INTERFACE)
target_include_directories(EmbeddedUtilities INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/eLog>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ring>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bit>
)

# Make mutex_common.h available to all modules
target_include_directories(eLog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ring PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

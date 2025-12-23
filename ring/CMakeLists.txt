add_library(ring STATIC ring.c)

target_include_directories(ring
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
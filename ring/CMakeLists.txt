add_library(ring STATIC ring.c)

target_include_directories(ring
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Ensure LTO compatibility for static library - ONLY for Release builds
target_compile_options(ring PRIVATE $<$<CONFIG:Release>:-flto> $<$<CONFIG:Release>:-ffat-lto-objects>)
target_link_options(ring PRIVATE $<$<CONFIG:Release>:-flto>)
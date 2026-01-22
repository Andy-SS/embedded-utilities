add_library(eLog STATIC eLog.c)

target_include_directories(eLog
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Ensure LTO compatibility for static library - ONLY for Release builds
target_compile_options(eLog PRIVATE $<$<CONFIG:Release>:-flto> $<$<CONFIG:Release>:-ffat-lto-objects>)
target_link_options(eLog PRIVATE $<$<CONFIG:Release>:-flto>)

add_library(eLog STATIC eLog.c)

target_include_directories(eLog
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

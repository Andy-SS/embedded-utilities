add_library(eLog STATIC eLog.c eLog.h)

target_include_directories(eLog
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../bit
)

# Link to bit utilities if needed
if(TARGET bit)
    target_link_libraries(eLog PRIVATE bit)
endif()


add_library(freemodbusForBeamMeasurement STATIC
    ${CMAKE_CURRENT_LIST_DIR}/functions/mbfunccoils.c
    ${CMAKE_CURRENT_LIST_DIR}/functions/mbfuncholding.c
    ${CMAKE_CURRENT_LIST_DIR}/functions/mbfuncinput.c
    ${CMAKE_CURRENT_LIST_DIR}/functions/mbutils.c
    ${CMAKE_CURRENT_LIST_DIR}/mb.c
    ${CMAKE_CURRENT_LIST_DIR}/rtu/mbcrc.c
    ${CMAKE_CURRENT_LIST_DIR}/rtu/mbrtu.c
)

target_link_libraries(freemodbusForBeamMeasurement pico_stdlib)

target_include_directories(freemodbusForBeamMeasurement PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/rtu/
    ${CMAKE_CURRENT_LIST_DIR}/include/
)

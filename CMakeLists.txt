cmake_minimum_required(VERSION 3.13)

project(beamMeasurementInInjectionLine C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(pico_sdk_import.cmake)

pico_sdk_init()

add_subdirectory(freemodbus)

# Target of the application
add_executable(beamMeasurementInInjectionLine
    ${CMAKE_CURRENT_LIST_DIR}/source/beamMeasurement.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/portevent.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/portserial.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/porttimer.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/modbusCustom.c
    ${CMAKE_CURRENT_LIST_DIR}/source/mainTimer.c
    ${CMAKE_CURRENT_LIST_DIR}/source/adcInputs.c
    ${CMAKE_CURRENT_LIST_DIR}/source/debuggingTools.c
    ${CMAKE_CURRENT_LIST_DIR}/source/compilationTime.c
)

target_include_directories(beamMeasurementInInjectionLine PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/freemodbus/include
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus
)

# Picotool/uf2 conversion in your SDK setup expects an .elf extension
set_target_properties(beamMeasurementInInjectionLine PROPERTIES SUFFIX ".elf")

# compilationTime.c rebuilding mechanism
add_custom_target(
    force_compilationTime_for_beamMeasurement ALL
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/phonyTimestamp
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/phonyTimestamp
    COMMENT "Touch phonyTimestamp to force recompilation of compilationTime.c"
)

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/compilationTime.c
    PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/phonyTimestamp
)

# Linking
target_link_libraries(beamMeasurementInInjectionLine
    freemodbusForBeamMeasurement
    pico_stdlib
    pico_time
    hardware_timer
    hardware_uart
    hardware_irq
    hardware_adc
)

add_custom_target(writeBuildTime ALL
  COMMAND /bin/sh -c "date '+%Y-%m-%d %H:%M:%S' > '${CMAKE_CURRENT_LIST_DIR}/bin/compilationTime.txt'"
  VERBATIM
)

# enable usb output, disable uart output
pico_enable_stdio_usb(beamMeasurementInInjectionLine 1)
pico_enable_stdio_uart(beamMeasurementInInjectionLine 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(beamMeasurementInInjectionLine)


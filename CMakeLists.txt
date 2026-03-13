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

# enable usb output, disable uart output
pico_enable_stdio_usb(beamMeasurementInInjectionLine 1)
pico_enable_stdio_uart(beamMeasurementInInjectionLine 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(beamMeasurementInInjectionLine)

# Archive UF2 in bin/<build-type>-<timestamp>-<git-hash>.uf2.
if(CMAKE_CONFIGURATION_TYPES)
    set(UF2_ARCHIVE_BUILD_KIND "$<LOWER_CASE:$<CONFIG>>")
else()
    string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
    if(BUILD_TYPE_LOWER STREQUAL "")
        set(BUILD_TYPE_LOWER "default")
    endif()
    set(UF2_ARCHIVE_BUILD_KIND "${BUILD_TYPE_LOWER}")
endif()

set(UF2_ARCHIVE_GIT_HASH "nogit")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=10 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        OUTPUT_VARIABLE UF2_ARCHIVE_GIT_HASH
        RESULT_VARIABLE UF2_ARCHIVE_GIT_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT UF2_ARCHIVE_GIT_RESULT EQUAL 0 OR UF2_ARCHIVE_GIT_HASH STREQUAL "")
        set(UF2_ARCHIVE_GIT_HASH "nogit")
    endif()
endif()

add_custom_command(TARGET beamMeasurementInInjectionLine POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_UF2="${CMAKE_CURRENT_BINARY_DIR}/beamMeasurementInInjectionLine.uf2"
        -DOUTPUT_DIR="${CMAKE_CURRENT_LIST_DIR}/bin"
        -DBUILD_KIND="${UF2_ARCHIVE_BUILD_KIND}"
        -DGIT_HASH="${UF2_ARCHIVE_GIT_HASH}"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/archive_uf2.cmake"
    COMMENT "Archive UF2 with build kind, timestamp and git hash"
)


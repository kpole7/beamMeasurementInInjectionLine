cmake_minimum_required(VERSION 3.13)

project(beamMeasurementInInjectionLine C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Check if building for Pico (cross-compile) or host (native Linux)
if(CMAKE_CROSSCOMPILING)
    include(pico_sdk_import.cmake)
    pico_sdk_init()
    
    add_subdirectory(freemodbus)
endif()

# Only build Pico application if cross-compiling
if(CMAKE_CROSSCOMPILING)
add_executable(beamMeasurementInInjectionLine
    ${CMAKE_CURRENT_LIST_DIR}/source/beamMeasurement.c
    ${CMAKE_CURRENT_LIST_DIR}/source/mainTimer.c
    ${CMAKE_CURRENT_LIST_DIR}/source/adcInputs.c
    ${CMAKE_CURRENT_LIST_DIR}/source/debuggingTools.c
    ${CMAKE_CURRENT_LIST_DIR}/source/compilationTime.c
    ${CMAKE_CURRENT_LIST_DIR}/source/sharedData.c
    ${CMAKE_CURRENT_LIST_DIR}/source/highLevelCtrl.c
    ${CMAKE_CURRENT_LIST_DIR}/source/auxiliaryFSMs.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/portevent.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/portserial.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/porttimer.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/modbusCustom.c
)

target_include_directories(beamMeasurementInInjectionLine PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/freemodbus/include
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus
)

target_compile_definitions(beamMeasurementInInjectionLine PRIVATE
    $<$<CONFIG:Debug>:APP_DEBUG_BUILD=1>
    $<$<CONFIG:Release>:APP_RELEASE_BUILD=1>
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

endif()  # PICO_SDK_PATH check

#==============================================================================
# Unit Tests for Host (Linux)
#==============================================================================
# Build unit tests when NOT cross-compiling (i.e., building on native host)
if(NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    
    # Build unit test executable for HighLevelCtrl
    add_executable(test_highLevelCtrl
        ${CMAKE_CURRENT_LIST_DIR}/source/highLevelCtrl.c
        ${CMAKE_CURRENT_LIST_DIR}/tests/test_highLevelCtrl.c
    )
    
    target_include_directories(test_highLevelCtrl PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
    )
    
    # Link with standard C library only (no Pico dependencies)
    target_link_libraries(test_highLevelCtrl PRIVATE m)
    
    # Register test
    add_test(NAME HighLevelCtrl COMMAND test_highLevelCtrl)

    # Build unit test executable for AuxiliaryFSMs
    add_executable(test_auxiliaryFSMs
        ${CMAKE_CURRENT_LIST_DIR}/source/auxiliaryFSMs.c
        ${CMAKE_CURRENT_LIST_DIR}/tests/test_auxiliaryFSMs.c
    )

    target_include_directories(test_auxiliaryFSMs PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
    )

    target_link_libraries(test_auxiliaryFSMs PRIVATE m)

    add_test(NAME AuxiliaryFSMs COMMAND test_auxiliaryFSMs)
endif()



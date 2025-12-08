add_subdirectory(freemodbus)

add_executable(beamMeasurementInInjectionLine 
	beamMeasurement.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/portevent.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/portserial.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/porttimer.c
    ${CMAKE_CURRENT_LIST_DIR}/portfreemodbus/modbusCustom.c
    spiCommunication.c
    debuggingTools.c
)

# pull in common dependencies
target_link_libraries(beamMeasurementInInjectionLine
	freemodbusForBeamMeasurement
	pico_stdlib
	pico_time
	hardware_timer
	hardware_uart
	hardware_irq
	hardware_spi
)

# enable usb output, disable uart output
pico_enable_stdio_usb(beamMeasurementInInjectionLine 1)
pico_enable_stdio_uart(beamMeasurementInInjectionLine 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(beamMeasurementInInjectionLine)

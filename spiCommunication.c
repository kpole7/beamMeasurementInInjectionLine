// This source code file was written by K.O. (2025 - 2026)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "masterConfig.h"
#include "modbusConfig.h"
#include "spiCommunication.h"
#include "debuggingTools.h"

//..............................................................................
// Definitions of constants
//..............................................................................


// SPI hardware clock frequency
#define SPI_CLOCK_FREQUENCY				1000000ul

// The line 'LD' of the power source is connected to GPIO 22
#define CURRENT_SOURCE_DATA_LATCH_PORT	22

//..............................................................................
// Local variables
//..............................................................................


spi_inst_t* PS_spi = spi_default;

//..............................................................................
// Definitions of interface functions
//..............................................................................

// Initialization of hardware peripherals and initialization of variables that
// are used to communicate with the power source via SPI.
void initializeSPI(void){
	spi_init(PS_spi, SPI_CLOCK_FREQUENCY);

	gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
	gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
	gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
	gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);

	gpio_init(CURRENT_SOURCE_DATA_LATCH_PORT);
	gpio_set_dir(CURRENT_SOURCE_DATA_LATCH_PORT, GPIO_OUT);

}


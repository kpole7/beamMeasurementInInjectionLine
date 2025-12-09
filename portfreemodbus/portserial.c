// FreeModbus Libary: BARE Port
// Copyright (C) 2006 Christian Walter <wolti@sil.at>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
// File: $Id$


// The file has been modified by K.Olejarczyk.


// ----------------------- Platform includes --------------------------------
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

#include "../../portfreemodbus/port.h"

// ----------------------- Modbus includes ----------------------------------
#include "mb.h"
#include "mbport.h"
#include "../debuggingTools.h" //K.O. modification
#include "../masterConfig.h" //K.O. modification

#ifdef VARIABLE_POINTERS_TO_FUNCTIONS_NOT_ALLOWED //K.O. modification
#include "mbrtu.h"
#endif

// ----------------------- Static functions ---------------------------------
static void prvvUARTxISR( void );

// ----------------------- Static variables ---------------------------------
//static uart_inst_t*     uart            = NULL;	//K.O. modification
static volatile bool    is_rx           = TRUE;
static volatile bool    is_tx           = FALSE;

// ----------------------- Start implementation -----------------------------
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
#if MODBUS_DEBUG_MODE
	logAddEvent("ser Enab", (uint16_t)xRxEnable);
#endif
//K.O.	uart_set_irq_enables( MODBUS_UART_ID, xRxEnable, xTxEnable );
    is_rx = xRxEnable;

//K.O.    int de_level = xTxEnable ? MB_DE_GPIO_ACTIVE : MB_DE_GPIO_INACTIVE;
//K.O.    gpio_put( MB_DE_GPIO, de_level );
    is_tx = xTxEnable;
}


void xMBPortSerialPoll(void)
{
	if(is_tx){
#if MODBUS_DEBUG_MODE
		logOnceAddEvent("ser Poll1",0xFFFFu);
#endif

		if(uart_is_writable( MODBUS_UART_ID )) //K.O. modification
		{
#ifndef VARIABLE_POINTERS_TO_FUNCTIONS_NOT_ALLOWED //K.O. modification
			pxMBFrameCBTransmitterEmpty(  );
#else
#if MODBUS_DEBUG_MODE
			logAddEvent("ser Poll2", 0xFFFFu);
#endif // MODBUS_DEBUG_MODE

			xMBRTUTransmitFSM(  );
#endif // VARIABLE_POINTERS_TO_FUNCTIONS_NOT_ALLOWED
    	}
	}
}

// This function was rewritten by K.O. based on the original one.
BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
    uart_init(MODBUS_UART_ID, MODBUS_BAUD_RATE);
    gpio_set_function(MODBUS_UART_TX_PIN, UART_FUNCSEL_NUM(MODBUS_UART_ID, MODBUS_UART_TX_PIN));
    gpio_set_function(MODBUS_UART_RX_PIN, UART_FUNCSEL_NUM(MODBUS_UART_ID, MODBUS_UART_RX_PIN));
    uart_set_baudrate( MODBUS_UART_ID, MODBUS_BAUD_RATE );
    uart_set_hw_flow(MODBUS_UART_ID, false, false);
    uart_set_format( MODBUS_UART_ID, MODBUS_DATA_BITS, 1, MODBUS_PARITY );
    uart_set_fifo_enabled(MODBUS_UART_ID, false);

	irq_set_exclusive_handler(MODBUS_UART0_IRQ, prvvUARTxISR);
    irq_set_enabled(MODBUS_UART0_IRQ, true);
    uart_set_irq_enables( MODBUS_UART_ID, true, false );
    return true;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
    uart_putc_raw( MODBUS_UART_ID, ucByte ); // uart_putc is not good due to its CRLF support
//K.O.    uart_tx_wait_blocking(MODBUS_UART_ID);
    ModbusActiveLedLong=true; //K.O.
   return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
// Return the byte in the UARTs receive buffer. This function is called
// by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
    *pucByte = uart_getc( MODBUS_UART_ID ); //K.O.
    return TRUE;
}

#ifndef VARIABLE_POINTERS_TO_FUNCTIONS_NOT_ALLOWED //K.O.
// Create an interrupt handler for the transmit buffer empty interrupt
// (or an equivalent) for your target processor. This function should then
// call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
// a new character can be sent. The protocol stack will then call
// xMBPortSerialPutByte( ) to send the character.
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

// Create an interrupt handler for the receive interrupt for your target
// processor. This function should then call pxMBFrameCBByteReceived( ). The
// protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
// character.
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}
#endif

// This function was rewritten by K.O. based on the original one.
// This function is an interrupt handler.
/// @callgraph
/// @callergraph
static void prvvUARTxISR( void )
{
	if(ModbusAssertionFailed){
		is_rx = false;
	}
    if ( is_rx )
    {
#if MODBUS_DEBUG_MODE
    	logAddEvent("irq rx", 0xFFFFu);
#endif
#ifndef VARIABLE_POINTERS_TO_FUNCTIONS_NOT_ALLOWED
    	pxMBFrameCBByteReceived();
#else
    	xMBRTUReceiveFSM();
    	ModbusActiveLedShort=true; //K.O.
#endif
    }
    else{
    	while(uart_is_readable(MODBUS_UART_ID)){
    		(void)uart_getc(MODBUS_UART_ID);
    	}
    }
#if MODBUS_DEBUG_MODE
    logAddEvent("reti uart", 0xFFFFu);
#endif
}


/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006-2018 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "../../portfreemodbus/port.h" /* K.O. modification */
#include "pico/stdlib.h" /* K.O. modification */
#include "pico/sync.h"   /* K.O. modification */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbrtu.h"
#include "mbframe.h"

#include "mbcrc.h"
#include "mbport.h"
#include "../../debuggingTools.h" /* K.O. modification */



/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_CRC     2       /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RX_INIT,              /*!< Receiver is in initial state. */
    STATE_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_RX_RCV,               /*!< Frame is beeing received. */
    STATE_RX_ERROR              /*!< If the frame is invalid. */
} eMBRcvState;

typedef enum
{
    STATE_TX_IDLE,              /*!< Transmitter is in idle state. */
    STATE_TX_XMIT               /*!< Transmitter is in transfer state. */
} eMBSndState;

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBSndState eSndState;
static atomic_uint_fast16_t eRcvState; /* K.O. modification */

volatile UCHAR  ucRTUBuf[MB_SER_PDU_SIZE_MAX];

static volatile UCHAR *pucSndBufferCur;
static volatile USHORT usSndBufferCount;

static volatile USHORT usRcvBufferPos;

static critical_section_t ModbusRtuCriticalSection; /* K.O. modification */

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBRTUInit( UCHAR ucSlaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    ULONG           usTimerT35_50us;

    ( void )ucSlaveAddress;
    /* ENTER_CRITICAL_SECTION(  );    K.O. modification */
    critical_section_init( &ModbusRtuCriticalSection ); /* K.O. modification */
    critical_section_enter_blocking( &ModbusRtuCriticalSection ); /* K.O. modification */

    /* Modbus RTU uses 8 Databits. */
    if( xMBPortSerialInit( ucPort, ulBaudRate, 8, eParity ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }
    else
    {
        /* If baudrate > 19200 then we should use the fixed timer values
         * t35 = 1750us. Otherwise t35 must be 3.5 times the character time.
         */
        if( ulBaudRate > 19200 )
        {
            usTimerT35_50us = 35;       /* 1800us. */
        }
        else
        {
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( Baudrate / 11 )
             *             = 11 * Ticks_per_1s / Baudrate
             *             = 220000 / Baudrate
             * The reload for t3.5 is 1.5 times this value and similary
             * for t3.5.
             */
            usTimerT35_50us = ( 7UL * 220000UL ) / ( 2UL * ulBaudRate );
        }
        if( xMBPortTimersInit( ( USHORT ) usTimerT35_50us ) != TRUE )
        {
            eStatus = MB_EPORTERR;
        }
    }
    /* EXIT_CRITICAL_SECTION(  );    K.O. modification */
    critical_section_exit( &ModbusRtuCriticalSection ); /* K.O. modification */

    return eStatus;
}

void
eMBRTUStart( void )
{
    /* ENTER_CRITICAL_SECTION(  );    K.O. modification */
    critical_section_enter_blocking( &ModbusRtuCriticalSection ); /* K.O. modification */

    /* Initially the receiver is in the state STATE_RX_INIT. we start
     * the timer and if no character is received within t3.5 we change
     * to STATE_RX_IDLE. This makes sure that we delay startup of the
     * modbus protocol stack until the bus is free.
     */
    atomic_store_explicit( &eRcvState, STATE_RX_INIT, memory_order_release ); /* K.O. */
    vMBPortSerialEnable( TRUE, FALSE );
    vMBPortTimersEnable(  );

    /* EXIT_CRITICAL_SECTION(  );    K.O. modification */
    critical_section_exit( &ModbusRtuCriticalSection ); /* K.O. modification */
}

#ifdef NOT_USED_FUNCTIONS_ALLOWED /* K.O. */
void
eMBRTUStop( void )
{
    ENTER_CRITICAL_SECTION(  );
    vMBPortSerialEnable( FALSE, FALSE );
    vMBPortTimersDisable(  );
    EXIT_CRITICAL_SECTION(  );
}
#endif

eMBErrorCode
eMBRTUReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
/*     BOOL            xFrameReceived = FALSE;            K.O. */
    eMBErrorCode    eStatus = MB_ENOERR;

    /* ENTER_CRITICAL_SECTION(  );    K.O. modification */
    critical_section_enter_blocking( &ModbusRtuCriticalSection ); /* K.O. modification */

    if(usRcvBufferPos >= MB_SER_PDU_SIZE_MAX){/* K.O. */
    	atomic_store_explicit( &ModbusAssertionFailed, true, memory_order_release ); /* K.O. */
#if MODBUS_DEBUG_MODE
    	logAddEvent("Assert",1);/* K.O. */
#endif
        critical_section_exit( &ModbusRtuCriticalSection ); /* K.O. modification */
    	return(true);/* K.O. */
    }/* K.O. */
/* K.O.    assert( usRcvBufferPos < MB_SER_PDU_SIZE_MAX ); */

    /* Length and CRC check */
    if( ( usRcvBufferPos >= MB_SER_PDU_SIZE_MIN )
        && ( usMBCRC16( ( UCHAR * ) ucRTUBuf, usRcvBufferPos ) == 0 ) )
    {
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = ucRTUBuf[MB_SER_PDU_ADDR_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = ( USHORT )( usRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );

        /* Return the start of the Modbus PDU to the caller. */
        *pucFrame = ( UCHAR * ) & ucRTUBuf[MB_SER_PDU_PDU_OFF];
/*         xFrameReceived = TRUE;           K.O. */
    }
    else
    {
        eStatus = MB_EIO;
    }

    /* EXIT_CRITICAL_SECTION(  );    K.O. modification */
    critical_section_exit( &ModbusRtuCriticalSection ); /* K.O. modification */
    return eStatus;
}

eMBErrorCode
eMBRTUSend( UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;

    /* ENTER_CRITICAL_SECTION(  );    K.O. modification */
    critical_section_enter_blocking( &ModbusRtuCriticalSection ); /* K.O. modification */

    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if( atomic_load_explicit( &eRcvState, memory_order_acquire ) == STATE_RX_IDLE ) /* K.O. modification */
    {
        /* First byte before the Modbus-PDU is the slave address. */
        pucSndBufferCur = ( UCHAR * ) pucFrame - 1;
        usSndBufferCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pucSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        usSndBufferCount += usLength;

        /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
        usCRC16 = usMBCRC16( ( UCHAR * ) pucSndBufferCur, usSndBufferCount );


#if MODBUS_DEBUG_MODE /* K.O. */
//        if (SimulationFrameError != 0){
//        	usCRC16++;
//        }
#endif

        ucRTUBuf[usSndBufferCount++] = ( UCHAR )( usCRC16 & 0xFF );
        ucRTUBuf[usSndBufferCount++] = ( UCHAR )( usCRC16 >> 8 );

        /* Activate the transmitter. */
        eSndState = STATE_TX_XMIT;

#if MODBUS_DEBUG_MODE
       	logAddEvent("eMBRTUSnd", 0xFFFFu); /* K.O. */
#endif

        vMBPortSerialEnable( FALSE, TRUE );
    }
    else
    {
        eStatus = MB_EIO;
    }
    /* EXIT_CRITICAL_SECTION(  );    K.O. modification */
    critical_section_exit( &ModbusRtuCriticalSection ); /* K.O. modification */
    return eStatus;
}

/* This function is called by the UART interrupt handler K.O. */
BOOL
xMBRTUReceiveFSM( void )
{
    BOOL            xTaskNeedSwitch = FALSE;
    UCHAR           ucByte;

    if(eSndState != STATE_TX_IDLE){/* K.O. modification */
    	atomic_store_explicit( &ModbusAssertionFailed, true, memory_order_release ); /* K.O. */
#if MODBUS_DEBUG_MODE
    	logAddEvent("Assert",2);/* K.O. */
#endif
    	return(true);/* K.O. */
    }/* K.O. */
/* K.O.    assert( eSndState == STATE_TX_IDLE ); */

    /* Always read the character. */
    ( void )xMBPortSerialGetByte( ( CHAR * ) & ucByte );

    switch ( atomic_load_explicit( &eRcvState, memory_order_acquire )) /* K.O. modification */
    {
        /* If we have received a character in the init state we have to
         * wait until the frame is finished.
         */
    case STATE_RX_INIT:
        vMBPortTimersEnable(  );
        break;

        /* In the error state we wait until all characters in the
         * damaged frame are transmitted.
         */
    case STATE_RX_ERROR:
        vMBPortTimersEnable(  );
        break;

        /* In the idle state we wait for a new character. If a character
         * is received the t1.5 and t3.5 timers are started and the
         * receiver is in the state STATE_RX_RECEIVCE.
         */
    case STATE_RX_IDLE:
        usRcvBufferPos = 0;
        ucRTUBuf[usRcvBufferPos++] = ucByte;
        atomic_store_explicit( &eRcvState, STATE_RX_RCV, memory_order_release ); /* K.O. */

        /* Enable t3.5 timers. */
        vMBPortTimersEnable(  );
        break;

        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
    case STATE_RX_RCV:
        if( usRcvBufferPos < MB_SER_PDU_SIZE_MAX )
        {
            ucRTUBuf[usRcvBufferPos++] = ucByte;
        }
        else
        {
        	atomic_store_explicit( &eRcvState, STATE_RX_ERROR, memory_order_release ); /* K.O. */
        }
        vMBPortTimersEnable(  );
        break;
    }
    return xTaskNeedSwitch;
}

BOOL
xMBRTUTransmitFSM( void )
{
    BOOL            xNeedPoll = FALSE;

    if(atomic_load_explicit( &eRcvState, memory_order_acquire ) != STATE_RX_IDLE){/* K.O. */
    	atomic_store_explicit( &ModbusAssertionFailed, true, memory_order_release ); /* K.O. */
#if MODBUS_DEBUG_MODE
    	logAddEvent("Assert",3);/* K.O. */
#endif
    	return(true);/* K.O. */
    }/* K.O. */
/* K.O.    assert( eRcvState == STATE_RX_IDLE ); */

    switch ( eSndState )
    {
        /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
    case STATE_TX_IDLE:
        /* enable receiver/disable transmitter. */
        vMBPortSerialEnable( TRUE, FALSE );
        break;

    case STATE_TX_XMIT:
        /* check if we are finished. */
        if( usSndBufferCount != 0 )
        {
            xMBPortSerialPutByte( ( CHAR )*pucSndBufferCur );
            pucSndBufferCur++;  /* next byte in sendbuffer. */
            usSndBufferCount--;
        }
        else
        {
            xNeedPoll = xMBPortEventPost( EV_FRAME_SENT );
            /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
            vMBPortSerialEnable( TRUE, FALSE );
            eSndState = STATE_TX_IDLE;
        }
        break;
    }

    return xNeedPoll;
}

/** This function is called only by the timer ISR (prvvTIMERExpiredISR)    (K.O.) */
BOOL
xMBRTUTimerT35Expired( void )
{
    BOOL            xNeedPoll = FALSE;

    switch ( atomic_load_explicit( &eRcvState, memory_order_acquire ))  /* K.O. */
    {
        /* Timer t35 expired. Startup phase is finished. */
    case STATE_RX_INIT:
        xNeedPoll = xMBPortEventPost( EV_READY );
        break;

        /* A frame was received and t35 expired. Notify the listener that
         * a new frame was received. */
    case STATE_RX_RCV:
        xNeedPoll = xMBPortEventPost( EV_FRAME_RECEIVED );
        break;

        /* An error occured while receiving the frame. */
    case STATE_RX_ERROR:
        break;

        /* Function called in an illegal state. */
    default:
    	vMBPortTimersDisable(  );/* K.O. */
    	atomic_store_explicit( &ModbusAssertionFailed, true, memory_order_release ); /* K.O. */
#if MODBUS_DEBUG_MODE
    	logAddEvent("Assert",4);/* K.O. */
#endif
    	return(false);/* K.O. */
/* K.O.    	assert( ( eRcvState == STATE_RX_INIT ) ||
                 ( eRcvState == STATE_RX_RCV ) || ( eRcvState == STATE_RX_ERROR ) ); */
    }

    vMBPortTimersDisable(  );
    atomic_store_explicit( &eRcvState, STATE_RX_IDLE, memory_order_release ); /* K.O. */

    return xNeedPoll;
}

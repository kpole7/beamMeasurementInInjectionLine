/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "debuggingTools.h" /* K.O. modification */

/* ----------------------- Variables ----------------------------------------*/
static atomic_uint_fast16_t eQueuedEvent; /* K.O. */
static atomic_bool xEventInQueue; /* K.O. */

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortEventInit( void )
{
	atomic_store_explicit( &xEventInQueue, false, memory_order_release ); /* K.O. */
    return TRUE;
}

/* This function is called both in the main loop and in the timer ISR (prvvTIMERExpiredISR)     (K.O.) */
BOOL
xMBPortEventPost( eMBEventType eEvent )
{
	atomic_store_explicit( &xEventInQueue, true, memory_order_release ); /* K.O. */
	atomic_store_explicit( &eQueuedEvent, eEvent, memory_order_release ); /* K.O. */

#if MODBUS_DEBUG_MODE
    logAddEvent("Event",atomic_load_explicit( &eQueuedEvent, memory_order_acquire )); /* K.O. modification */
#endif

    return TRUE;
}

BOOL
xMBPortEventGet( eMBEventType * eEvent )
{
    BOOL            xEventHappened = FALSE;

    if( atomic_load_explicit( &xEventInQueue, memory_order_acquire )) /* K.O. */
    {
        *eEvent = atomic_load_explicit( &eQueuedEvent, memory_order_acquire ); /* K.O. */
        atomic_store_explicit( &xEventInQueue, false, memory_order_release ); /* K.O. */
        xEventHappened = TRUE;
    }
    return xEventHappened;
}

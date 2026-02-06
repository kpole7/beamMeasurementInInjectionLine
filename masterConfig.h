/// @file masterConfig.h
// This header file was written by K.O. (2025 - 2026)

#ifndef _MASTER_CONFIG_H
#define _MASTER_CONFIG_H

#define VARIABLE_POINTERS_TO_FUNCTIONS_NOT_ALLOWED
#undef NOT_USED_FUNCTIONS_ALLOWED

// This directive allows you to select one of the modes: debug (value '1') or release (value '0').
// The 'release' mode is tested according to the same scenario as the 'debug' mode, but the tests
// in the 'debug' mode are more extensive.
#define MODBUS_DEBUG_MODE			0

// This directive allows for printouts while processing Modbus commands
// (debugging Modbus state machine)
#define MODBUS_DEBUG_PRINT			0

#endif // _MASTER_CONFIG_H

/// @file masterConfig.h
// This header file was written by K.O. (2025 - 2026)

#ifndef _MASTER_CONFIG_H
#define _MASTER_CONFIG_H

#define VARIABLE_POINTERS_TO_FUNCTIONS_NOT_ALLOWED
#undef NOT_USED_FUNCTIONS_ALLOWED

// The concept that the data from the measurement path of the power supply should be divided by 2;
#define SCALE_MEASURED_CURRENT_IN_HALF		1

// Intervention shutdown of setpoints in various abnormal situations
#define INTERVENTION_SHUTDOWN_ON_POWER_OFF	0
#define INTERVENTION_SHUTDOWN_ON_LOCAL_MODE	0
#define INTERVENTION_SHUTDOWN_ON_SUM_ERROR	1
#define INTERVENTION_SHUTDOWN_ON_EXT_ERROR	1

// This directive allows you to select one of the modes: debug (value '1') or release (value '0').
// The 'release' mode is tested according to the same scenario as the 'debug' mode, but the tests
// in the 'debug' mode are more extensive.
#define MODBUS_DEBUG_MODE			0

// This directive allows for printouts while processing Modbus commands
// (debugging Modbus state machine)
#define MODBUS_DEBUG_PRINT			0

#endif // _MASTER_CONFIG_H

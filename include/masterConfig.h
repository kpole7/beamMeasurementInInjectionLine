/// @file masterConfig.h
// This header file was written by K.O. (2025 - 2026)

#ifndef MASTER_CONFIG_H
#define MASTER_CONFIG_H

#define VARIABLE_POINTERS_TO_FUNCTIONS_NOT_ALLOWED
#undef NOT_USED_FUNCTIONS_ALLOWED

// This directive allows for printouts while processing Modbus commands
// (debugging Modbus state machine)
#define MODBUS_DEBUG_PRINT 0

// In this mode, peripheral devices are ignored and overridden by software simulation
#define DEBUG_SIMULATION_MODE 0


#endif // MASTER_CONFIG_H

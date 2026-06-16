/// @file actuatorCtrl.c
/// @brief This module controls the state of the actuators (pneumatic valves and motor actuator)

#include "actuatorCtrl.h"
#include "sharedData.h"
#include "pico/stdlib.h"
#include <stdio.h>

//---------------------------------------------------------------------------------------------------
// Macro directives
//---------------------------------------------------------------------------------------------------

#define GPIO_FOR_VALVE_ACTUATOR_1 7
#define GPIO_FOR_VALVE_ACTUATOR_2 6
#define GPIO_FOR_MOTOR_ACTUATOR_IN 18
#define GPIO_FOR_MOTOR_ACTUATOR_OUT 19
#define GPIO_FOR_MOTOR_ACTUATOR_BRAKE 16

//---------------------------------------------------------------------------------------------------
// Local variables
//---------------------------------------------------------------------------------------------------

static bool StateValveActuator1;
static bool StateValveActuator2;
static bool StateMotorActuatorIn;
static bool StateMotorActuatorOut;
static bool StateMotorActuatorBrake;

//---------------------------------------------------------------------------------------------------
// Function definitions
//---------------------------------------------------------------------------------------------------

void initializeActuatorControl(void) {
	StateValveActuator1 = true; // initial state of the pneumatic valve #1 is 'inserted'
	StateValveActuator2 = true; // initial state of the pneumatic valve #2 is 'inserted'
	StateMotorActuatorIn = false;
	StateMotorActuatorOut = false;
	StateMotorActuatorBrake = false;

	gpio_init(GPIO_FOR_VALVE_ACTUATOR_1);
	gpio_set_dir(GPIO_FOR_VALVE_ACTUATOR_1, GPIO_OUT);
	gpio_put(GPIO_FOR_VALVE_ACTUATOR_1, StateValveActuator1);

	gpio_init(GPIO_FOR_VALVE_ACTUATOR_2);
	gpio_set_dir(GPIO_FOR_VALVE_ACTUATOR_2, GPIO_OUT);
	gpio_put(GPIO_FOR_VALVE_ACTUATOR_2, StateValveActuator2);

	gpio_init(GPIO_FOR_MOTOR_ACTUATOR_IN);
	gpio_set_dir(GPIO_FOR_MOTOR_ACTUATOR_IN, GPIO_OUT);
	gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, StateMotorActuatorIn);

	gpio_init(GPIO_FOR_MOTOR_ACTUATOR_OUT);
	gpio_set_dir(GPIO_FOR_MOTOR_ACTUATOR_OUT, GPIO_OUT);
	gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, StateMotorActuatorOut);

	gpio_init(GPIO_FOR_MOTOR_ACTUATOR_BRAKE);
	gpio_set_dir(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, GPIO_OUT);
	gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, StateMotorActuatorBrake);
}

void actuatorCtrlTick(void) {
	bool TrigValveActuator1 = ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)];
	bool TrigValveActuator2 = ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)];
	bool TrigMotorActuatorIn = ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)];
	bool TrigMotorActuatorOut = ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)];
	bool TrigMotorActuatorBrake = ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_BRAKE)];

	// just for testing purposes
	static bool DebugPrintoutsEnabled = false;
	if (((ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_PRINTOUTS)] & 2u) != 0u) != DebugPrintoutsEnabled) {
		DebugPrintoutsEnabled = ((ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_PRINTOUTS)] & 2u) != 0u);
		if (DebugPrintoutsEnabled) {
			printf("Debug printouts for actuator control enabled\r\n");
		}
		else{
			printf("Debug printouts for actuator control disabled\r\n");
		}
	}
	bool AnyTrigger = TrigValveActuator1 || TrigValveActuator2 || TrigMotorActuatorIn || TrigMotorActuatorOut || TrigMotorActuatorBrake;
	// printout the old states
	if (DebugPrintoutsEnabled && AnyTrigger) {
		printf("Actuator:   1    2   3in  out brake\r\n");
		printf("Old state:  %u    %u    %u    %u    %u\r\n", 
			StateValveActuator1, StateValveActuator2, StateMotorActuatorIn, StateMotorActuatorOut, StateMotorActuatorBrake);
	} // just for testing purposes

	StateValveActuator1 = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)];
	StateValveActuator2 = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)];
	StateMotorActuatorIn = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)];
	StateMotorActuatorOut = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)];
	StateMotorActuatorBrake = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_BRAKE)];

	// just for testing purposes; printout the triggers and requested states
	if (DebugPrintoutsEnabled && AnyTrigger) {
		printf("Trigger:    %u    %u    %u    %u    %u\r\n", TrigValveActuator1, TrigValveActuator2, TrigMotorActuatorIn, TrigMotorActuatorOut, TrigMotorActuatorBrake);
		printf("Request:    %u    %u    %u    %u    %u\r\n", StateValveActuator1, StateValveActuator2, StateMotorActuatorIn, StateMotorActuatorOut, StateMotorActuatorBrake);
	} // just for testing purposes

	if (TrigValveActuator1) {
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] = false;
		gpio_put(GPIO_FOR_VALVE_ACTUATOR_1, StateValveActuator1); // Pneumatic valve #1
	}

	if (TrigValveActuator2) {
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] = false;
		gpio_put(GPIO_FOR_VALVE_ACTUATOR_2, StateValveActuator2); // Pneumatic valve #2
	}

	if (TrigMotorActuatorIn) {
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)] = false;
		if (StateMotorActuatorIn){
			// 'insert' command

			if (!StateMotorActuatorOut && !StateMotorActuatorBrake) { 
				// only allow activation if not already activated in the opposite direction and not braking

				// make sure the opposite direction is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
				StateMotorActuatorOut = false;

				// make sure the breaking is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, false);
				StateMotorActuatorBrake = false;

				// activate the 'insert' command
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, true);
				StateMotorActuatorIn = true;
			}
			else{
				// emergency stop
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
				StateMotorActuatorIn = false;
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
				StateMotorActuatorOut = false;
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, false);
				StateMotorActuatorBrake = false;
			}
		}
		else{
			// stop 'insert' command
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
			StateMotorActuatorIn = false;
		}
	}
	
	if (TrigMotorActuatorOut) {
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)] = false;
		if (StateMotorActuatorOut){
			// 'extract' command

			if (!StateMotorActuatorIn && !StateMotorActuatorBrake) { 
				// only allow activation if not already activated in the opposite direction and not braking

				// make sure the opposite direction is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
				StateMotorActuatorIn = false;

				// make sure the breaking is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, false);
				StateMotorActuatorBrake = false;

				// activate the 'extract' command
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, true);
				StateMotorActuatorOut = true;
			}
			else{
				// emergency stop
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
				StateMotorActuatorIn = false;
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
				StateMotorActuatorOut = false;
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, false);
				StateMotorActuatorBrake = false;
			}
		}
		else{
			// stop 'extract' command
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
			StateMotorActuatorOut = false;
		}
	}
	
	if (TrigMotorActuatorBrake) {
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_BRAKE)] = false;
		gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
		StateMotorActuatorIn = false;

		gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
		StateMotorActuatorOut = false;

		if (StateMotorActuatorBrake) {
			// command 'brake'
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true);
		}
		else{
			// release brake
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, false);
			StateMotorActuatorBrake = false;
		}
	}

	// just for testing purposes
	if (DebugPrintoutsEnabled && AnyTrigger) {
		printf("New state:  %u    %u    %u    %u    %u\r\n\r\n", StateValveActuator1, StateValveActuator2, StateMotorActuatorIn, StateMotorActuatorOut, StateMotorActuatorBrake);
	} // just for testing purposes
}


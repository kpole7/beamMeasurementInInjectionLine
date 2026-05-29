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
#define GPIO_FOR_MOTOR_ACTUATOR_IN 19
#define GPIO_FOR_MOTOR_ACTUATOR_OUT 18
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
	StateValveActuator1 = false;
	StateValveActuator2 = false;
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
	bool stateValveActuator1 = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)];
	bool stateValveActuator2 = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)];
	bool stateMotorActuatorIn = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)];
	bool stateMotorActuatorOut = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)];
	bool stateMotorActuatorBrake = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_BRAKE)];

	bool trigValveActuator1 = ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)];
	bool trigValveActuator2 = ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)];
	bool trigMotorActuatorIn = ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)];
	bool trigMotorActuatorOut = ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)];
	bool trigMotorActuatorBrake = ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_BRAKE)];

	// just for testing purposes
	bool anyTrigger = trigValveActuator1 || trigValveActuator2 || trigMotorActuatorIn || trigMotorActuatorOut || trigMotorActuatorBrake;
	if (((ModbusHoldingRegisters[MODBUS_ADDR_DEBUG_PRINTOUTS] & 2u) != 0u) && anyTrigger) {
		printf("Actuator:   1    2   3in  out brake\r\n");
		printf("Old state:  %u    %u    %u    %u    %u\r\n", StateValveActuator1, StateValveActuator2, StateMotorActuatorIn, StateMotorActuatorOut, StateMotorActuatorBrake);
		printf("Trigger:    %u    %u    %u    %u    %u\r\n", trigValveActuator1, trigValveActuator2, trigMotorActuatorIn, trigMotorActuatorOut, trigMotorActuatorBrake);
		printf("Command:    %u    %u    %u    %u    %u\r\n", stateValveActuator1, stateValveActuator2, stateMotorActuatorIn, stateMotorActuatorOut, stateMotorActuatorBrake);
	} // just for testing purposes

	if (trigValveActuator1) {
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] = false;
		gpio_put(GPIO_FOR_VALVE_ACTUATOR_1, stateValveActuator1); // Pneumatic valve #1
		StateValveActuator1 = stateValveActuator1;
	}

	if (trigValveActuator2) {
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] = false;
		gpio_put(GPIO_FOR_VALVE_ACTUATOR_2, stateValveActuator2); // Pneumatic valve #2
		StateValveActuator2 = stateValveActuator2;
	}

	if (trigMotorActuatorIn) {
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)] = false;
		if (stateMotorActuatorIn){
			// 'insert' command

			if (!StateMotorActuatorOut && !StateMotorActuatorBrake) { 
				// only allow activation if not already activated in the opposite direction and not braking

				// make sure the opposite direction is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
				StateMotorActuatorOut = false;

				// make sure the breaking is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true); // low-level activation
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
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true); // low-level activation
				StateMotorActuatorBrake = false;
			}
		}
		else{
			// stop 'insert' command
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
			StateMotorActuatorIn = false;
		}
	}

	if (trigMotorActuatorOut) {
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)] = false;
		if (stateMotorActuatorOut){
			// 'extract' command

			if (!StateMotorActuatorIn && !StateMotorActuatorBrake) { 
				// only allow activation if not already activated in the opposite direction and not braking

				// make sure the opposite direction is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
				StateMotorActuatorIn = false;

				// make sure the breaking is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true); // low-level activation
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
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true); // low-level activation
				StateMotorActuatorBrake = false;
			}
		}
		else{
			// stop 'extract' command
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
			StateMotorActuatorOut = false;
		}
	}
	
	if (trigMotorActuatorBrake) {
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_BRAKE)] = false;
		gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
		StateMotorActuatorIn = false;

		gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
		StateMotorActuatorOut = false;

		if (stateMotorActuatorBrake) {
			// command 'brake'
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, !stateMotorActuatorBrake); // low-level activation
			StateMotorActuatorBrake = stateMotorActuatorBrake;
		}
		else{
			// release brake
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true); // low-level activation
			StateMotorActuatorBrake = false;
		}
	}

	// just for testing purposes
	if (((ModbusHoldingRegisters[MODBUS_ADDR_DEBUG_PRINTOUTS] & 2u) != 0u) && anyTrigger) {
		printf("New state:  %u    %u    %u    %u    %u\r\n", StateValveActuator1, StateValveActuator2, StateMotorActuatorIn, StateMotorActuatorOut, StateMotorActuatorBrake);
		printf("\r\n");
	} // just for testing purposes
}


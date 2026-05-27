/// @file actuatorCtrl.c
/// @brief This module controls the state of the actuators (pneumatic valves and motor actuator)

#include "actuatorCtrl.h"
#include "sharedData.h"
#include "pico/stdlib.h"

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

static bool OldStateValveActuator1;
static bool OldStateValveActuator2;
static bool OldStateMotorActuatorIn;
static bool OldStateMotorActuatorOut;
static bool OldStateMotorActuatorBrake;

//---------------------------------------------------------------------------------------------------
// Function definitions
//---------------------------------------------------------------------------------------------------

void initializeActuatorControl(void) {
	OldStateValveActuator1 = false;
	OldStateValveActuator2 = false;
	OldStateMotorActuatorIn = false;
	OldStateMotorActuatorOut = false;
	OldStateMotorActuatorBrake = false;

	gpio_init(GPIO_FOR_VALVE_ACTUATOR_1);
	gpio_set_dir(GPIO_FOR_VALVE_ACTUATOR_1, GPIO_OUT);
	gpio_put(GPIO_FOR_VALVE_ACTUATOR_1, OldStateValveActuator1);

	gpio_init(GPIO_FOR_VALVE_ACTUATOR_2);
	gpio_set_dir(GPIO_FOR_VALVE_ACTUATOR_2, GPIO_OUT);
	gpio_put(GPIO_FOR_VALVE_ACTUATOR_2, OldStateValveActuator2);

	gpio_init(GPIO_FOR_MOTOR_ACTUATOR_IN);
	gpio_set_dir(GPIO_FOR_MOTOR_ACTUATOR_IN, GPIO_OUT);
	gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, OldStateMotorActuatorIn);

	gpio_init(GPIO_FOR_MOTOR_ACTUATOR_OUT);
	gpio_set_dir(GPIO_FOR_MOTOR_ACTUATOR_OUT, GPIO_OUT);
	gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, OldStateMotorActuatorOut);

	gpio_init(GPIO_FOR_MOTOR_ACTUATOR_BRAKE);
	gpio_set_dir(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, GPIO_OUT);
	gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, OldStateMotorActuatorBrake);
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

	if (trigValveActuator1) {
		gpio_put(GPIO_FOR_VALVE_ACTUATOR_1, stateValveActuator1); // Pneumatic valve #1
		OldStateValveActuator1 = stateValveActuator1;
	}

	if (trigValveActuator2) {
		gpio_put(GPIO_FOR_VALVE_ACTUATOR_2, stateValveActuator2); // Pneumatic valve #2
		OldStateValveActuator2 = stateValveActuator2;
	}

	if (trigMotorActuatorIn) {
		if (stateMotorActuatorIn){
			// 'insert' command

			if (!OldStateMotorActuatorOut && !stateMotorActuatorBrake) { 
				// only allow activation if not already activated in the opposite direction and not braking

				// make sure the opposite direction is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
				OldStateMotorActuatorOut = false;

				// make sure the breaking is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true); // low-level activation
				OldStateMotorActuatorBrake = false;

				// activate the 'insert' command
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, true);
				OldStateMotorActuatorIn = true;
			}
			else{
				// emergency stop
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
				OldStateMotorActuatorIn = false;
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
				OldStateMotorActuatorOut = false;
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true); // low-level activation
				OldStateMotorActuatorBrake = false;
			}
		}
		else{
			// stop 'insert' command
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
			OldStateMotorActuatorIn = false;
		}
	}

	if (trigMotorActuatorOut) {
		if (stateMotorActuatorOut){
			// 'extract' command

			if (!OldStateMotorActuatorIn && !stateMotorActuatorBrake) { 
				// only allow activation if not already activated in the opposite direction and not braking

				// make sure the opposite direction is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
				OldStateMotorActuatorIn = false;

				// make sure the breaking is deactivated
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true); // low-level activation
				OldStateMotorActuatorBrake = false;

				// activate the 'extract' command
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, true);
				OldStateMotorActuatorOut = true;
			}
			else{
				// emergency stop
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
				OldStateMotorActuatorIn = false;
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
				OldStateMotorActuatorOut = false;
				gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true); // low-level activation
				OldStateMotorActuatorBrake = false;
			}
		}
		else{
			// stop 'extract' command
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
			OldStateMotorActuatorOut = false;
		}
	}
	
	if (trigMotorActuatorBrake) {
		gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);
		OldStateMotorActuatorIn = false;

		gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);
		OldStateMotorActuatorOut = false;

		if (stateMotorActuatorBrake) {
			// command 'brake'
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, !stateMotorActuatorBrake); // low-level activation
			OldStateMotorActuatorBrake = stateMotorActuatorBrake;
		}
		else{
			// release brake
			gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, true); // low-level activation
			OldStateMotorActuatorBrake = false;
		}
	}
}


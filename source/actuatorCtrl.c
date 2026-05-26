/// @file actuatorCtrl.c
/// @brief This module controls the state of the actuators (pneumatic valves and motor actuator)

#include "actuatorCtrl.h"
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
	gpio_init(GPIO_FOR_VALVE_ACTUATOR_1);
	gpio_set_dir(GPIO_FOR_VALVE_ACTUATOR_1, GPIO_OUT);
	gpio_put(GPIO_FOR_VALVE_ACTUATOR_1, false);

	gpio_init(GPIO_FOR_VALVE_ACTUATOR_2);
	gpio_set_dir(GPIO_FOR_VALVE_ACTUATOR_2, GPIO_OUT);
	gpio_put(GPIO_FOR_VALVE_ACTUATOR_2, false);

	gpio_init(GPIO_FOR_MOTOR_ACTUATOR_IN);
	gpio_set_dir(GPIO_FOR_MOTOR_ACTUATOR_IN, GPIO_OUT);
	gpio_put(GPIO_FOR_MOTOR_ACTUATOR_IN, false);

	gpio_init(GPIO_FOR_MOTOR_ACTUATOR_OUT);
	gpio_set_dir(GPIO_FOR_MOTOR_ACTUATOR_OUT, GPIO_OUT);
	gpio_put(GPIO_FOR_MOTOR_ACTUATOR_OUT, false);

	gpio_init(GPIO_FOR_MOTOR_ACTUATOR_BRAKE);
	gpio_set_dir(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, GPIO_OUT);
	gpio_put(GPIO_FOR_MOTOR_ACTUATOR_BRAKE, false);
}
#include "src/agilis_controller.h"
#include <stdio.h>


int main() {

	printf("Trying to initialize actuators...\n");
	int controller = init_actuators(actuator_tty); // actuator_tty = "/dev/ttyUSB0"

	printf("Checking each channel/axis status...\n");
	get_axis_channel_status(1, 1, controller);
	get_axis_channel_status(2, 1, controller);
	get_axis_channel_status(1, 2, controller);
	get_axis_channel_status(2, 2, controller);

	printf("Moving each channel/axis by 1 step...\n");
	step_to_axis_channel(1, 1, 1, controller);
	step_to_axis_channel(1, 2, 1, controller);
	step_to_axis_channel(1, 1, 2, controller);
	step_to_axis_channel(1, 2, 2, controller);

	return 0;
}
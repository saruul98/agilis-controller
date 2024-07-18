#ifndef AGILIS_CONTROLLER_H
#define AGILIS_CONTROLLER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <thread>
#include <mutex>
#include <time.h>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>

#define actuator_tty "/dev/ttyUSB0"

std::string read_cpp(int commfd);
void write_cpp(std::string buf, int commfd);
void write_comm(std::string buf, int commfd, bool check_for_error);
bool read_limit(int commfd, int axis);
int init_actuators(std::string tty);
void step_to_axis_channel(int steps,int axis,int cha, int controller);
void move_to_axis_channel(int pos, int axis, int cha, int controller);
void get_axis_channel_status(int axis, int channel, int controller);

#endif // AGILIS_CONTROLLER_H
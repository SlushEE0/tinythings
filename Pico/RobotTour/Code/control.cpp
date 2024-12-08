#include "include/motor.h"
#include "include/pid.hpp"

void control_init() {
  // left and right motors without the pins
  motor_init();

  t_motor* motorL = &motor_l;
  t_motor* motorR = &motor_r;
}

void control_mainLoop() {
  motor_mainLoop();
}
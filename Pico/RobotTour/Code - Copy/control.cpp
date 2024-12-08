#include "include/motor.h"
#include "include/pid.hpp"

void control_init() {
  // left and right motors without the pins
  t_motor motor_l = { .isReversed = false,
                      .volts = 0,
                      .speed = 0,
                      .desired_speed = 0,
                      .encoder_counts = 0,
                      .desired_counts = 0,
                      .pin_encA = 20,
                      .pin_encB = 21,
                      .pin_control = 0,
                      .pid_runningI = 0,
                      .pid_runningD = 0 };

  t_motor motor_r = { .isReversed = false,
                      .volts = 0,
                      .speed = 0,
                      .desired_speed = 0,
                      .encoder_counts = 0,
                      .desired_counts = 0,
                      .pin_encA = 0,
                      .pin_encB = 0,
                      .pin_control = 0,
                      .pid_runningI = 0,
                      .pid_runningD = 0 };

  motor_init(&motor_l, &motor_r);
}
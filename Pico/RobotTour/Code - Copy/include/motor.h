#ifndef MOTOR_H
#define MOTOR_H

#include "constants.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

typedef struct t_motor {
  bool isReversed;

  double maxVolts;

  double volts;
  double speed;
  double desired_speed;

  int encoder_counts;
  int desired_counts;

  int pin_encA;
  int pin_encB;
  int pin_control;

  int pid_runningI;
  int pid_runningD;
} t_motor;

// Global motor instances
extern t_motor *motor_l;
extern t_motor *motor_r;

// Function prototypes
void ISR_R(uint trigger, uint32_t events);
void ISR_L(uint trigger, uint32_t events);
void register_int(int gpio, void (*ISR)(uint, uint32_t));
void runPid(t_motor *motor);
void encoders_init();
void motor_mainLoop();
void motor_init(t_motor *motorR, t_motor *motorL);

#endif // MOTOR_H
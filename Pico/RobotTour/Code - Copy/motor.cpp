#include "hardware/timer.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
// #include "hardware/pwm.h"
#include "include/constants.h"
#include "include/motor.h"

static int isr_runs = 0;

// left and right motors without the pins
t_motor *motor_l;
t_motor *motor_r;

void ISR_R(uint trigger, uint32_t events) {
  int deltaCounts = 0;

  if (trigger == motor_l->pin_encA) {
    if (gpio_get(motor_l->pin_encB)) {
      deltaCounts = 1;
    } else {
      deltaCounts = 1;
    }
  }

  if (trigger == motor_l->pin_encB) {
    if (gpio_get(motor_l->pin_encA)) {
      deltaCounts = 1;
    } else {
      deltaCounts = 1;
    }
  }

  if (motor_l->isReversed) {
    deltaCounts *= -1;
  }

  motor_l->encoder_counts += deltaCounts;

  printf("ISR %d. GPIO: %d\n", isr_runs++, trigger);
  printf("motor_l->encoder_counts: %d\n", motor_l->encoder_counts);
}

void ISR_L(uint trigger, uint32_t events) {
  if (trigger == motor_r->pin_encA) {
    if (gpio_get(motor_r->pin_encB)) {
      motor_r->encoder_counts += 1;
    } else {
      motor_r->encoder_counts -= 1;
    }
  }

  if (trigger == motor_r->pin_encB) {
    if (gpio_get(motor_r->pin_encA)) {
      motor_r->encoder_counts -= 1;
    } else {
      motor_r->encoder_counts += 1;
    }
  }

  printf("ISR %d. GPIO: %d\n", isr_runs++, trigger);
  printf("motor_r->encoder_counts: %d\n", motor_r->encoder_counts);
}

void register_int(int gpio, void (*ISR)(uint, uint32_t)) {
  gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_FALL, true, ISR);
}

void runPid(t_motor *motor) {
  double error = motor->desired_counts - motor->encoder_counts;

  motor->pid_runningI += error;

  double derivative = error - motor->pid_runningD;
  motor->pid_runningD = error;

  double output =
    error * MOTOR_kP + motor->pid_runningI * MOTOR_kI + derivative * MOTOR_kD;

  // set motor speed
  motor->speed = output;
}

void encoders_init() {
  gpio_set_dir(motor_l->pin_encA, false);
  gpio_set_dir(motor_l->pin_encB, false);

  gpio_set_dir(motor_r->pin_encA, false);
  gpio_set_dir(motor_r->pin_encB, false);

  // register interrupts for encoder
  register_int(motor_l->pin_encA, &ISR_L);
  register_int(motor_l->pin_encB, &ISR_L);

  register_int(motor_r->pin_encA, &ISR_R);
  register_int(motor_r->pin_encB, &ISR_R);
}

void motor_mainLoop() {}

void motor_init(t_motor *motorL, t_motor *motorR) {
  // gpio_set_function(motor_l->pin_control, GPIO_FUNC_PWM);
  // gpio_set_function(motor_r->pin_control, GPIO_FUNC_PWM);
  motor_l = motorL;
  motor_r = motorR;

  encoders_init();
}
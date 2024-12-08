#include "include/motor.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "include/constants.h"
#include "pico/stdlib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int isr_runs = 0;

// left and right motors without the pins
t_motor motor_l = { .isReversed = false,
                    .maxVolts = 12,
                    .volts = 2,
                    .speed = 0,
                    .desired_speed = 0,
                    .encoder_counts = 0,
                    .desired_counts = 0,
                    .pin_encA = 20,
                    .pin_encB = 21,
                    .pin_pwm = 7,
                    .pin_in1 = 8,
                    .pin_in2 = 9,
                    .brake = false };

t_motor motor_r = { .isReversed = false,
                    .maxVolts = 12,
                    .volts = 0,
                    .speed = 0,
                    .desired_speed = 0,
                    .encoder_counts = 0,
                    .desired_counts = 0,
                    .pin_encA = 0,
                    .pin_encB = 0,
                    .pin_pwm = 0,
                    .pin_in1 = 0,
                    .pin_in2 = 0,
                    .brake = false };

void ISR_R(uint trigger, uint32_t events) {
  int deltaCounts = 0;

  if (trigger == motor_l.pin_encA) {
    if (gpio_get(motor_l.pin_encB)) {
      deltaCounts = 1;
    } else {
      deltaCounts = -1;
    }
  }

  if (trigger == motor_l.pin_encB) {
    if (gpio_get(motor_l.pin_encA)) {
      deltaCounts = -1;
    } else {
      deltaCounts = 1;
    }
  }

  if (motor_l.isReversed) {
    deltaCounts *= -1;
  }

  motor_l.encoder_counts += deltaCounts;

  printf("ISR %d. GPIO: %d\n", isr_runs++, trigger);
  printf("motor_l.encoder_counts: %d\n", motor_l.encoder_counts);
}

void ISR_L(uint trigger, uint32_t events) {
  if (trigger == motor_r.pin_encA) {
    if (gpio_get(motor_r.pin_encB)) {
      motor_r.encoder_counts += 1;
    } else {
      motor_r.encoder_counts -= 1;
    }
  }

  if (trigger == motor_r.pin_encB) {
    if (gpio_get(motor_r.pin_encA)) {
      motor_r.encoder_counts -= 1;
    } else {
      motor_r.encoder_counts += 1;
    }
  }

  printf("ISR %d. GPIO: %d\n", isr_runs++, trigger);
  printf("motor_r.encoder_counts: %d\n", motor_r.encoder_counts);
}

void register_int(int gpio, void (*ISR)(uint, uint32_t)) {
  gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_FALL, true, ISR);
}

void encoders_init() {
  gpio_set_dir(motor_l.pin_encA, false);
  gpio_set_dir(motor_l.pin_encB, false);

  gpio_set_dir(motor_r.pin_encA, false);
  gpio_set_dir(motor_r.pin_encB, false);

  // register interrupts for encoder
  register_int(motor_l.pin_encA, &ISR_L);
  register_int(motor_l.pin_encB, &ISR_L);

  register_int(motor_r.pin_encA, &ISR_R);
  register_int(motor_r.pin_encB, &ISR_R);
}

void motor_mainLoop() {
  if (motor_l.volts * (motor_l.isReversed ? -1 : 1) > 0) {
    gpio_put(motor_l.pin_in1, 1);
    gpio_put(motor_l.pin_in2, 0);
  } else if (motor_l.volts * (motor_l.isReversed ? -1 : 1) < 0) {
    gpio_put(motor_l.pin_in1, 0);
    gpio_put(motor_l.pin_in2, 1);
  } else {
    if (motor_l.brake) {
      gpio_put(motor_l.pin_in1, 1);
      gpio_put(motor_l.pin_in2, 1);
    } else {
      gpio_put(motor_l.pin_in1, 0);
      gpio_put(motor_l.pin_in2, 0);
    }

    pwm_set_gpio_level(motor_l.pin_pwm, 0);
  }

  printf("ye: %d\n", gpio_get(motor_l.pin_in1));
  printf("ye2: %d\n", gpio_get(motor_l.pin_in2));

  pwm_set_gpio_level(
    motor_l.pin_pwm,
    1024 * (abs(motor_l.volts) / motor_l.maxVolts)
  );
}

void motor_init() {
  // initalize all pins
  gpio_init(motor_l.pin_pwm);
  gpio_init(motor_l.pin_in1);
  gpio_init(motor_l.pin_in2);
  gpio_set_function(motor_l.pin_pwm, GPIO_FUNC_PWM);
  gpio_set_dir(motor_l.pin_in1, GPIO_OUT);
  gpio_set_dir(motor_l.pin_in2, GPIO_OUT);

  gpio_init(motor_r.pin_pwm);
  gpio_init(motor_r.pin_in1);
  gpio_init(motor_r.pin_in2);
  gpio_set_function(motor_r.pin_pwm, GPIO_FUNC_PWM);
  gpio_set_dir(motor_r.pin_in1, GPIO_OUT);
  gpio_set_dir(motor_r.pin_in2, GPIO_OUT);

  uint l_slice1 = pwm_gpio_to_slice_num(motor_l.pin_pwm);

  pwm_set_wrap(l_slice1, 1024);
  pwm_set_chan_level(l_slice1, pwm_gpio_to_channel(motor_l.pin_pwm), 0);
  pwm_set_enabled(l_slice1, true);
  pwm_set_clkdiv(l_slice1, 1.0);

  encoders_init();
}
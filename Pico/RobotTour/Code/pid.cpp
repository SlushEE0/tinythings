#include "include/pid.hpp"
#include "math.h"

PID::PID(double Kp, double Ki, double Kd, t_motor *motor) {
  this->Kp = Kp;
  this->Ki = Ki;
  this->Kd = Kd;
  this->motor = motor;
  this->prevError = 0;
  this->integral = 0;
}

double PID::clamp(double value) {
  if (std::abs(value) > motor->maxVolts) {
    return motor->maxVolts * (value >= 0 ? 1 : -1);
  }
  return value;
}

double PID::calculate(double processVariable, double setpoint) {
  double error = setpoint - processVariable;
  integral += error;
  double derivative = (error - prevError);
  prevError = error;
  return clamp(Kp * error + Ki * integral + Kd * derivative);
}

double PID::calculate() {
  return calculate(motor->encoder_counts, motor->desired_counts);
}

double PID::calculate(double processVariable) {
  return calculate(processVariable, motor->desired_counts);
}
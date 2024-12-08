#ifndef PID_H
#define PID_H

#include "motor.h"

class PID {
private:
  double Kp, Ki, Kd;
  double prevError, integral;
  double dt;
  t_motor *motor;

public:
  PID(double Kp, double Ki, double Kd, t_motor *motor);

  double clamp(double value);

  double calculate(double processVariable, double setpoint);
  double calculate();
  double calculate(double processVariable);
};

#endif // PID_H
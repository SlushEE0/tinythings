#ifndef CONTROL_H
#define CONTROL_H

#include "motor.h"
#include "pid.hpp"

// Function prototypes
void control_init();
void control_mainLoop();

#endif // CONTROL_H
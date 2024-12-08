#ifndef _USS_H
#define _USS_H

#include "freertos/FreeRTOS.h"

static inline double uss_pulseWidthToCM(int64_t width);
void init_GPIO();
double uss_measure();
bool garage_getIsOpen();

#endif
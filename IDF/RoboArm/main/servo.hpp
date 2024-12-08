#ifndef H_SERVO
#define H_SERVO

#include <functional>

#include <gpio_cxx.hpp>

using namespace idf;
using namespace std;

extern "C" {

typedef struct t_servoCfg {
  int us_min;
  int us_max;
  int period;
  double us_perDeg;

  GPIO_Output* gpio;
  double* degrees;
} t_servoCfg;

void runCycle(t_servoCfg* config);
}

#endif
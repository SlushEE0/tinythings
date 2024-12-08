#include <cstdlib>
#include <functional>
#include <thread>

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

void runCycle(t_servoCfg* config)
{
  int us_duty = (*(config->degrees) * (config->us_perDeg) + config->us_min);
  if (us_duty > config->us_max)
    us_duty = config->us_max;

  int waitTime = *(config->degrees) * ((config->us_perDeg) * 10);

  (*(config->gpio)).set_high();
  this_thread::sleep_for(chrono::microseconds(us_duty));
  (*(config->gpio)).set_low();
  this_thread::sleep_for(chrono::microseconds(waitTime));
  this_thread::sleep_for(chrono::microseconds(config->period));
};
}
#ifndef _CONFIG
#define _CONFIG

#define CONFIG_WIFI_SSID ("gadem_spec")
#define CONFIG_WIFI_PASS ("cenT!100")
// #define CONFIG_WIFI_PS

// #define CONFIG_USE_HTTPS

#define CONFIG_ECHO_PIN (22)
#define CONFIG_TRIG_PIN (23)
#define CONFIG_RELAY_PIN (11)

#define CONFIG_US_TIMEOUT (2000)
#define CONFIG_US_PING_LEN (10)
#define CONFIG_RELAY_TIME (2500)

// When garage is open, how far is it? Ideally, you should add another 20 cm to this measurement
#define CONFIG_GARAGE_DISTANCE_CM (10)

#endif

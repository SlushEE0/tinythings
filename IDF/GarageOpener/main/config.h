#ifndef _CFG
#define _CFG

#define SPEED_SOUND (343) // m/s

#define TRIG_PIN (23)
#define ECHO_PIN (22)
#define RELAY_PIN (11)
#define WIFI_SSID "gadem_spec"
#define WIFI_PASS "cenT!100"

#define US_TIMEOUT (2000)
#define US_PING_LEN (10000)

#define MS_POLLING_RATE (500)

// When garage is open, how far is it? Ideally, you should add another 20 cm to this measurement
#define CONFIG_GARAGE_DISTANCE_CM (10)

#endif

// BAS: Battery Service.

/***** BAS common defines *****/
#define BATTERY_MIN_LEVEL        0
#define BATTERY_MAX_LEVEL        100

#define CHECK_BATTERY_LEVEL(val) ((val) > BATTERY_MIN_LEVEL && (val) < BATTERY_MAX_LEVEL)

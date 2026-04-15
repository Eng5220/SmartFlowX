/**
 * @file lgpio.h (MOCK)
 * @brief Mock lgpio header for cross-compilation testing without hardware.
 *
 * Provides type definitions and function stubs matching the real lgpio API
 * so that FlowMeter.cpp can compile without the real Raspberry Pi library.
 * The actual function implementations are in flow_meter_standalone.cpp.
 */

#ifndef LGPIO_H
#define LGPIO_H

#define LG_RISING_EDGE 1

struct lgGpioAlert_t {
    int report;
    int nfyHandle;
};
typedef lgGpioAlert_t* lgGpioAlert_p;

typedef void (*lgGpioAlertsFunc_t)(int, lgGpioAlert_p, void*);

// Function declarations (defined in flow_meter_standalone.cpp)
int lgGpioClaimInput(int handle, int flags, int pin);
int lgGpioClaimAlert(int handle, int flags, int edge, int pin, int debounce);
int lgGpioSetAlertsFunc(int handle, int pin, lgGpioAlertsFunc_t func, void* userdata);
int lgGpioFree(int handle, int pin);

#endif // LGPIO_H

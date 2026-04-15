#ifndef PINCONFIG_H
#define PINCONFIG_H

// ─── GPIO Pin Assignments (BCM numbering) ────────────────────────────────────

// DC Water Pump (via TIP122 transistor)
constexpr int PUMP_PIN = 18;   // GPIO18 (BCM)

// YF-S401 Flow Meter
// Wiring: Red→Pin2(5V)  Black→Pin9(GND)  Yellow→Pin11(GPIO17)
// Signal is open-collector; internal pull-up enabled in FlowMeter::setupGpio()
constexpr int FLOW_PIN = 17;   // GPIO17 — BCM17 — Physical Pin 11

// ─── Timing Settings ─────────────────────────────────────────────────────────
// NOTE: Set HOLD_TIME_SECONDS back to 3 for production use.
// Timing requirement: system must detect cup and begin filling within 50 ms
// of a proximity event. The GestureSensor polls at 50 ms; the Timer fires
// every LOOP_INTERVAL_MS for state-machine ticks.
constexpr int HOLD_TIME_SECONDS  = 1;     // cup must be held steady (seconds)
constexpr int LOOP_INTERVAL_MS   = 100;   // state-machine tick interval (ms)

// ─── Flow Settings ───────────────────────────────────────────────────────────
// YF-S401 spec: ~5880 pulses/litre  →  1000 mL / 5880 pulses ≈ 0.1701 mL/pulse
// CALIBRATED 2026-04-02: 2711 pulses measured for 200 ml actual
// → 200 / 2711 = 0.073774 ml/pulse (sensor runs ~2× datasheet spec)
constexpr double ML_PER_PULSE     = 0.073774;  // calibrated
constexpr double TARGET_VOLUME_ML = 200.0;     // dispense 200 ml per fill

// ─── GPIO Chip ───────────────────────────────────────────────────────────────
// /dev/gpiochip4 for Raspberry Pi 5; use 0 for Pi 1–4.
constexpr unsigned int GPIO_CHIP_NO = 4u;  // libgpiod chip number

// ─── I2C Bus / Addresses ─────────────────────────────────────────────────────
// APDS-9960 Gesture Sensor: SDA→Pin3 (GPIO2), SCL→Pin5 (GPIO3), VCC→3.3V
// I2C address fixed at 0x39 (hardware).
constexpr int GESTURE_I2C_BUS     = 1;     // /dev/i2c-1
constexpr int GESTURE_I2C_ADDR    = 0x39;  // APDS-9960 fixed address
constexpr int GESTURE_THRESHOLD   = 40;    // proximity trigger threshold (0–255)

// LCD1602 with PCF8574T I2C backpack — shares bus with gesture sensor (different address)
// Run `i2cdetect -y 1` to confirm address: default = 0x27, alt (jumper) = 0x3F
constexpr int LCD_I2C_BUS     = 1;     // /dev/i2c-1
constexpr int LCD_I2C_ADDRESS = 0x27;  // PCF8574T default

#endif // PINCONFIG_H

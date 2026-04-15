#ifndef PINCONFIG_H
#define PINCONFIG_H

// ─── GPIO Pin Assignments (BCM numbering) ────────────────────────────────────
// HC-SR04 Ultrasonic Sensor
constexpr int TRIG_PIN = 23;   // GPIO23 — trigger output
constexpr int ECHO_PIN = 24;   // GPIO24 — echo input

// DC Water Pump (via TIP121 transistor)
constexpr int PUMP_PIN = 27;   // GPIO27 — pump control output

// YF-S401 Flow Meter
constexpr int FLOW_PIN = 17;   // GPIO17 — pulse input (to be confirmed)

// ─── Sensor Settings ─────────────────────────────────────────────────────────
constexpr double TARGET_DISTANCE_CM = 12.0;   // target bottle distance
constexpr double DISTANCE_TOLERANCE_CM = 1.0; // ±1 cm tolerance (11–13 cm)

// ─── Timing Settings ─────────────────────────────────────────────────────────
// NOTE: Set back to 60 for the real demo — reduced to 5 for rapid testing.
constexpr int HOLD_TIME_SECONDS = 5;           // bottle must be stable (seconds)
constexpr int LOOP_INTERVAL_MS = 1000;         // main loop interval (1 second)

// ─── Flow Settings ───────────────────────────────────────────────────────────
constexpr double ML_PER_PULSE = 2.25;          // calibration constant
constexpr double TARGET_VOLUME_ML = 500.0;     // target fill volume

// ─── GPIO Chip ───────────────────────────────────────────────────────────────
constexpr int GPIO_CHIP = 4;                   // /dev/gpiochip4 — Raspberry Pi 5 header

#endif // PINCONFIG_H

# LcdDisplay Code Explanation

This document explains the functionality of the `LcdDisplay` component in the AquaFlow project, specifically focusing on `LcdDisplay.h` and `LcdDisplay.cpp`.

## Overview
The `LcdDisplay` class is a hardware driver for a **Freenove I2C LCD 1602** display (16 columns, 2 rows). It controls an underlying HD44780 LCD controller attached via a PCF8574T I2C backpack. This setup allows the Raspberry Pi to control the full display using only **4 wires** (5V power, Ground, SDA data, and SCK clock) over the I2C interface.

The class implements the `IHardwareDevice` interface, ensuring it integrates cleanly and identically to other hardware components in the system like the `PumpController` and `FlowMeter`.

## Key Features

1. **Direct POSIX I2C Communication**: 
   The code uses the standard Linux kernel I2C interface (`/dev/i2c-N` and `ioctl()`) to communicate directly with the device. This is a very clean approach because it avoids pulling in bulky third-party libraries (like WiringPi), relying entirely on standard C++ POSIX functions.

2. **Flicker-Free Text Updating**: 
   A common issue with basic LCD code is that clearing the screen before printing new text causes a visible, annoying "flicker". This driver solves that in the `print()` method by intelligently padding every string with spaces so it is exactly 16 characters long. This perfectly overwrites the previous text without ever needing to clear the screen!

## Core Methods

### `init()`
- Opens the `/dev/i2c-N` file corresponding to the configured bus number to establish the connection.
- Executes the highly specific initialization sequence required by the HD44780 datasheet: sending pulses and waiting exact microsecond delays to put the display into "4-bit mode".
- Configures the display for 2 lines and 5x8 dot characters.
- Turns on the display, turns on the backlight, clears the screen, and sets the cursor to automatically advance to the right.

### `shutdown()`
- Safely and cleanly powers down the display.
- Clears the screen and prints a farewell message: `AquaFlow \ Shutting down...`.
- Waits 1.5 seconds so the user can read the message, then dynamically cuts power to the backlight and closes the Linux file descriptor.

### `print(int row, const std::string& text)`
- Utility method to write text to either the top row (`0`) or bottom row (`1`).
- Manipulates the string to be precisely 16 characters long.

### `showVolume(double volumeML)`
- Formats the current dispensed liquid volume into a readable string (e.g., `Vol: 123.4 ml`).
- Specifically updates the bottom row (row 1). This is meant to be called rapidly (e.g. 1Hz timer) while the machine is actively filling a bottle.

### `showStatus(const std::string& state, double volumeML, int bottles)`
- High-level method called by the main system whenever the machine changes states (e.g., from `IDLE` to `FILLING`).
- **Row 0**: Writes the current system state (e.g. `FILLING`). It has a smart optimization where it skips writing if the state hasn't changed, saving processing and I2C bandwidth!
- **Row 1**: Contextually decides what to show. If it's `FILLING`, it shows the live volume. If the machine is resting (`IDLE`, `DONE`, `WAITING`), it displays the total count of bottles dispensed so far.

## Low-Level Hardware Interface (The bottom half)
The bottom portion of the code (`send`, `write4bits`, `pulseEnable`, etc.) handles the grueling electronics protocol required for the backpack chip:
- Because the I2C chip only has 8 pins, and the LCD requires more, the driver uses "4-bit mode". It splits every byte of data into two 4-bit chunks ("nibbles") and sends them one after the other.
- The `pulseEnable()` function accurately toggles the LCD's `En` (Enable) pin HIGH then LOW in the span of exactly 51 microseconds, effectively telling the chip "LATCH THIS DATA NOW".
- The `expanderWrite()` function always intelligently ORs your data with the backlight bit. This ensures that every time you send data, the code doesn't accidentally turn off the backlight.

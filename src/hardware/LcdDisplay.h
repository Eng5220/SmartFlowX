#pragma once

#include "IHardwareDevice.h"

#include <cstdint>
#include <string>

/**
 * @brief Freenove I2C LCD 1602 driver (HD44780 + PCF8574T backpack).
 *
 * Uses the Linux kernel i2c-dev interface (/dev/i2c-N + ioctl) — zero
 * third-party dependencies beyond POSIX.  Implements IHardwareDevice so it
 * integrates identically to PumpController and FlowMeter.
 *
 * Wiring (4 wires only):
 *   LCD VDD → Pi Pin 2  (5V)
 *   LCD VSS → Pi Pin 6  (GND)
 *   LCD SDA → Pi Pin 3  (GPIO2 / I2C1_SDA)
 *   LCD SCK → Pi Pin 5  (GPIO3 / I2C1_SCL)
 *
 * Typical usage:
 * @code
 *   LcdDisplay lcd(LCD_I2C_BUS, LCD_I2C_ADDRESS);
 *   lcd.init();
 *
 *   // Orchestrator formats the text; driver just renders it:
 *   lcd.print(0, "FILLING");
 *   lcd.print(1, "Vol: 123.4 ml");
 * @endcode
 */
class LcdDisplay : public IHardwareDevice {
public:
    /**
     * @param i2cBus   Linux I2C bus number (1 → /dev/i2c-1).
     * @param i2cAddr  I2C address of the PCF8574T backpack (0x27 or 0x3F).
     */
    LcdDisplay(int i2cBus, int i2cAddr);
    ~LcdDisplay() override;

    LcdDisplay(const LcdDisplay&) = delete;
    LcdDisplay& operator=(const LcdDisplay&) = delete;
    LcdDisplay(LcdDisplay&&) = delete;
    LcdDisplay& operator=(LcdDisplay&&) = delete;

    // ── IHardwareDevice ───────────────────────────────────────────────────────

    /** @brief Open I2C device, run HD44780 4-bit init sequence, enable backlight. */
    bool init() override;

    /** @brief Turn off backlight and release the I2C file descriptor. */
    void shutdown() override;

    // ── Display API ───────────────────────────────────────────────────────────

    /** @brief Clear all characters and return cursor to home. */
    void clear();

    /**
     * @brief Write a string to one row (always from column 0).
     *
     * The string is padded with spaces to 16 characters, overwriting any
     * previous content on that row without flickering the whole display.
     *
     * @param row   0 = top row, 1 = bottom row.
     * @param text  Text to display (truncated to 16 chars if longer).
     */
    void print(int row, const std::string& text);

private:
    // ── HD44780 low-level helpers ─────────────────────────────────────────────
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void send(uint8_t value, uint8_t mode);
    void write4bits(uint8_t nibble);
    void pulseEnable(uint8_t data);
    void expanderWrite(uint8_t data);
    void setCursor(int row, int col);

    // ── PCF8574T bit positions ────────────────────────────────────────────────
    static constexpr uint8_t BACKLIGHT   = 0x08;
    static constexpr uint8_t NOBACKLIGHT = 0x00;
    static constexpr uint8_t En          = 0x04;  // Enable strobe
    static constexpr uint8_t Rw          = 0x02;  // Read/Write (always 0 = write)
    static constexpr uint8_t Rs          = 0x01;  // Register Select (0=cmd, 1=data)

    // ── HD44780 commands ──────────────────────────────────────────────────────
    static constexpr uint8_t LCD_CLEARDISPLAY    = 0x01;
    static constexpr uint8_t LCD_RETURNHOME      = 0x02;
    static constexpr uint8_t LCD_ENTRYMODESET    = 0x04;
    static constexpr uint8_t LCD_DISPLAYCONTROL  = 0x08;
    static constexpr uint8_t LCD_FUNCTIONSET     = 0x20;
    static constexpr uint8_t LCD_SETDDRAMADDR    = 0x80;

    // ── HD44780 flags ─────────────────────────────────────────────────────────
    static constexpr uint8_t LCD_ENTRYLEFT  = 0x02;
    static constexpr uint8_t LCD_DISPLAYON  = 0x04;
    static constexpr uint8_t LCD_4BITMODE   = 0x00;
    static constexpr uint8_t LCD_2LINE      = 0x08;
    static constexpr uint8_t LCD_5x8DOTS    = 0x00;

    int  busNo_;
    int  addr_;
    int  fd_          = -1;
    bool initialised_ = false;
    bool backlightOn_ = true;
};

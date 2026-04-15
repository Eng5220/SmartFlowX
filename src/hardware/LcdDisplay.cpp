#include "hardware/LcdDisplay.h"
#include "utils/Logger.h"

#include <array>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <thread>
#include "utils/Logger.h"

// ─────────────────────────────────────────────────────────────────────────────

namespace {
    void delayUs(int us) {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }
    void delayMs(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

// ── Constructor / Destructor ──────────────────────────────────────────────────

LcdDisplay::LcdDisplay(int i2cBus, int i2cAddr)
    : busNo_(i2cBus), addr_(i2cAddr) {}

LcdDisplay::~LcdDisplay() {
    shutdown();
}

// ── IHardwareDevice ───────────────────────────────────────────────────────────

bool LcdDisplay::init() {
    if (initialised_) return true;

    try {
        const std::string devPath = "/dev/i2c-" + std::to_string(busNo_);
        fd_ = open(devPath.c_str(), O_RDWR);
        if (fd_ < 0)
            throw std::runtime_error("Cannot open " + devPath + ": " + strerror(errno));

        if (ioctl(fd_, I2C_SLAVE, addr_) < 0)
            throw std::runtime_error("ioctl I2C_SLAVE failed: " + std::string(strerror(errno)));

        // ── HD44780 4-bit initialisation sequence (datasheet §4.4) ────────────
        // Give VCC time to stabilise; pull all lines LOW
        delayMs(50);
        expanderWrite(0);
        delayMs(1000);

        // Three 8-bit mode wake-up pulses before switching to 4-bit
        write4bits(0x30); delayMs(5);
        write4bits(0x30); delayMs(5);
        write4bits(0x30); delayUs(150);

        // Switch to 4-bit mode
        write4bits(0x20);

        // Configure: 4-bit, 2 lines, 5×8 dots
        writeCommand(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS);

        // Display ON, cursor off, blink off
        writeCommand(LCD_DISPLAYCONTROL | LCD_DISPLAYON);

        // Clear and move to home
        writeCommand(LCD_CLEARDISPLAY);
        delayMs(2);                             // clear takes up to 1.52 ms

        // Entry mode: cursor moves right, display doesn't shift
        writeCommand(LCD_ENTRYMODESET | LCD_ENTRYLEFT);

        backlightOn_ = true;
        initialised_ = true;

        std::ostringstream addrHex;
        addrHex << std::hex << addr_;
        Logger::info("LcdDisplay initialised (bus=" + std::to_string(busNo_) +
                     ", addr=0x" + addrHex.str() + ")");
        return true;

    } catch (const std::exception& e) {
        Logger::error(std::string("LcdDisplay::init() error: ") + e.what());
        if (fd_ >= 0) { close(fd_); fd_ = -1; }
        return false;
    }
}

void LcdDisplay::shutdown() {
    try {
        if (initialised_) {
            // Turn off the backlight if it is still functionally connected
            backlightOn_ = false;
            expanderWrite(0x00);
        }
    } catch (const std::exception& e) {
        Logger::warn("LcdDisplay shutdown error swallowed: " + std::string(e.what()));
    }

    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
    initialised_ = false;

    Logger::info("LcdDisplay shut down.");
}

// ── Display API ───────────────────────────────────────────────────────────────

void LcdDisplay::clear() {
    if (!initialised_) return;
    try {
        writeCommand(LCD_CLEARDISPLAY);
        delayMs(2);
    } catch (const std::exception& e) {
        Logger::error("LcdDisplay transport fault (clear): " + std::string(e.what()));
        initialised_ = false;
    }
}

void LcdDisplay::print(int row, const std::string& text) {
    if (!initialised_) return;

    try {
        // Pad or truncate to exactly 16 characters — overwrites previous content
        // on that row without a full clear (no flicker)
        std::string line = text;
        line.resize(16, ' ');

        setCursor(row, 0);
        for (char c : line)
            writeData(static_cast<uint8_t>(c));
    } catch (const std::exception& e) {
        Logger::error("LcdDisplay transport fault (print): " + std::string(e.what()));
        initialised_ = false;
    }
}

// ── HD44780 low-level helpers ─────────────────────────────────────────────────

void LcdDisplay::setCursor(int row, int col) {
    // DDRAM addresses: row 0 starts at 0x00, row 1 starts at 0x40
    static constexpr std::array<uint8_t, 2> rowOffsets = { 0x00, 0x40 };
    writeCommand(LCD_SETDDRAMADDR | (static_cast<uint8_t>(col) + rowOffsets[row & 0x01]));
}

void LcdDisplay::writeCommand(uint8_t cmd)  { send(cmd,  0);   }
void LcdDisplay::writeData(uint8_t data)    { send(data, Rs);  }

void LcdDisplay::send(uint8_t value, uint8_t mode) {
    // Split byte into high nibble then low nibble (4-bit mode)
    uint8_t hi = value & 0xF0;
    uint8_t lo = (value << 4) & 0xF0;
    write4bits(hi | mode);
    write4bits(lo | mode);
}

void LcdDisplay::write4bits(uint8_t nibble) {
    expanderWrite(nibble);
    pulseEnable(nibble);
}

void LcdDisplay::pulseEnable(uint8_t data) {
    expanderWrite(data | En);   // En HIGH — latch data
    delayUs(1);                 // >450 ns pulse width
    expanderWrite(data & ~En);  // En LOW
    delayUs(50);                // >37 µs command execution time
}

void LcdDisplay::expanderWrite(uint8_t data) {
    // OR in the backlight bit before sending to PCF8574T
    uint8_t byte = data | (backlightOn_ ? BACKLIGHT : NOBACKLIGHT);
    if (::write(fd_, &byte, 1) != 1) {
        throw std::runtime_error("I2C expander write failed");
    }
}







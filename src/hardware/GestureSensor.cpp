#include "hardware/GestureSensor.h"
#include <array>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <chrono>

#include "utils/Logger.h"

// ── APDS-9960 Register Map ──────────────────────────────────────────────────
// Ported from Adafruit_APDS9960.h (v1.3.1, BSD licence)
#define APDS9960_ENABLE   0x80
#define APDS9960_ATIME    0x81
#define APDS9960_CONTROL  0x8F
#define APDS9960_ID       0x92
#define APDS9960_STATUS   0x93
#define APDS9960_PDATA    0x9C
#define APDS9960_GPENTH   0xA0
#define APDS9960_GEXTH    0xA1
#define APDS9960_GCONF1   0xA2
#define APDS9960_GCONF2   0xA3
#define APDS9960_GPULSE   0xA6
#define APDS9960_GCONF3   0xAA
#define APDS9960_GCONF4   0xAB
#define APDS9960_GFLVL    0xAE
#define APDS9960_GSTATUS  0xAF
#define APDS9960_AICLEAR  0xE7
#define APDS9960_GFIFO_U  0xFC

// Device ID (Adafruit library checks for 0xAB only; we additionally accept
// the alternate Adafruit silicon revision 0xA8 and the DollaTek clone 0x9E)
#define BRAND_ID_ADAFRUIT_1 0xAB
#define BRAND_ID_ADAFRUIT_2 0xA8
#define BRAND_ID_DOLLATEK   0x9E

// Gesture direction codes (matching Adafruit library)
#define GESTURE_UP    0x01
#define GESTURE_DOWN  0x02
#define GESTURE_LEFT  0x03
#define GESTURE_RIGHT 0x04

const int POLL_INTERVAL_MS = 50;

// ── Helper: milliseconds since epoch (replaces Arduino millis()) ─────────────
static uint64_t nowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

// ─────────────────────────────────────────────────────────────────────────────

GestureSensor::GestureSensor(int i2cBus, int i2cAddr, int threshold)
    : i2cBus_(i2cBus), i2cAddr_(i2cAddr), threshold_(threshold) {}

GestureSensor::~GestureSensor() {
    shutdown();
}

// ── init() — Ported from Adafruit_APDS9960::begin() ─────────────────────────
// This follows the exact same register configuration sequence as the Adafruit
// Arduino library (v1.3.1), adapted for Linux I2C.
// Reference: Adafruit_APDS9960_Library-1.3.1/Adafruit_APDS9960.cpp, lines 90-137
bool GestureSensor::init() {
    if (running_) return true;

    try {
        const std::string devPath = "/dev/i2c-" + std::to_string(i2cBus_);
        fd_ = open(devPath.c_str(), O_RDWR);
        if (fd_ < 0) {
            throw std::runtime_error("Cannot open " + devPath + ": " + strerror(errno));
        }

        if (ioctl(fd_, I2C_SLAVE, i2cAddr_) < 0) {
            throw std::runtime_error("ioctl I2C_SLAVE failed: " + std::string(strerror(errno)));
        }

        // ── Verify device ID ─────────────────────────────────────────────────
        uint8_t id = readRegister(APDS9960_ID);
        if (id != BRAND_ID_ADAFRUIT_1 && id != BRAND_ID_ADAFRUIT_2 && id != BRAND_ID_DOLLATEK) {
            std::ostringstream msg;
            msg << "Warning: Unknown APDS9960 device ID: 0x" << std::hex << static_cast<int>(id);
            Logger::warn(msg.str());
        }

        // ── Adafruit begin() sequence ────────────────────────────────────────
        // Step 1: Set default integration time (10ms) and ADC gain (4x)
        // ATIME = 256 - (10 / 2.78) = 256 - 3 = 253
        writeRegister(APDS9960_ATIME, 253);
        // CONTROL: AGAIN=4x (0x01), PGAIN=0, LDRIVE=0
        writeRegister(APDS9960_CONTROL, 0x01);

        // Step 2: Disable all engines to start clean
        // ENABLE: GEN=0, PEN=0, AEN=0, PON=0  (everything off)
        writeRegister(APDS9960_ENABLE, 0x00);

        // Step 3: Clear interrupts
        // Write to AICLEAR register (command-only, clears all non-gesture interrupts)
        {
            uint8_t reg = APDS9960_AICLEAR;
            if (::write(fd_, &reg, 1) != 1) {
                Logger::warn("Failed to clear APDS9960 interrupts");
            }
        }

        // Step 4: Power cycle — disable then re-enable with delays
        // (Adafruit: enable(false); delay(10); enable(true); delay(10);)
        // NOTE: These are hardware-mandated I2C device settling delays during
        // one-time initialisation, NOT real-time timing mechanisms. The APDS-9960
        // datasheet requires >= 5.7 ms power-on stabilisation time.
        writeRegister(APDS9960_ENABLE, 0x00);  // PON=0
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        writeRegister(APDS9960_ENABLE, 0x01);  // PON=1
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // ── Clear stale gesture FIFO data from any previous session ─────────
        // GCONF4 bit 2 (GFIFO_CLR) flushes the FIFO; it auto-clears after.
        // Reference: CircuitPython apds9960.py init: _set_bit(GCONF4, 0x04, True)
        writeRegister(APDS9960_GCONF4, 0x04);

        // ── Configure Gesture Engine ─────────────────────────────────────────
        // Register values are drawn from both the Adafruit Arduino library
        // (v1.3.1) and the CircuitPython library, choosing whichever defaults
        // were proven to give the best range and reliability.

        // GCONF3: use all four photodiode dimensions (Arduino default)
        writeRegister(APDS9960_GCONF3, 0x00);

        // GCONF1 = (GFIFOTH << 6) | (GEXMSK << 2) | GEXPERS
        // GFIFOTH=2 (8 datasets before GINT) — CircuitPython default.
        // GEXPERS=2 (4 consecutive exit cycles before leaving gesture mode) —
        //   prevents premature exit on transient dips below the exit threshold.
        // GEXMSK=0 (no photodiode masking).
        // → (2 << 6) | (0 << 2) | 2 = 0x82
        writeRegister(APDS9960_GCONF1, 0x82);

        // GCONF2 = (GGAIN << 5) | (GLDRIVE << 3) | GWTIME
        // GGAIN=2 (4x), GLDRIVE=0 (100mA), GWTIME=1 (2.8ms)
        // Both Arduino and CircuitPython use GGAIN=2; CircuitPython adds GWTIME=1.
        // → (2 << 5) | (0 << 3) | 1 = 0x41
        writeRegister(APDS9960_GCONF2, 0x41);

        // GPENTH: gesture engine entry threshold (proximity counts).
        // CircuitPython uses 5 (very sensitive, allows detection at ~20 cm).
        // The Arduino library uses 50, which restricts range to ~5 cm.
        // We use 5 to match the working CircuitPython behaviour.
        writeRegister(APDS9960_GPENTH, 5);

        // GEXTH: gesture engine exit threshold (all photodiodes below this → exit).
        // CircuitPython sets 30 (0x1E). Our old code and the Arduino library
        // never wrote this register, leaving it at the power-on default of 0.
        writeRegister(APDS9960_GEXTH, 30);

        // GPULSE: GPLEN=3 (32us), GPULSE=9 (10 pulses) → (3 << 6) | 9 = 0xC9
        // Arduino library default. More IR energy = better detection range.
        writeRegister(APDS9960_GPULSE, 0xC9);

        // ── Enable gesture engine ────────────────────────────────────────────
        // ENABLE: PON=1, PEN=1, GEN=1 → 0x01 | 0x04 | 0x40 = 0x45
        writeRegister(APDS9960_ENABLE, 0x45);

        // GCONF4: GMODE=1 (force gesture engine entry)
        writeRegister(APDS9960_GCONF4, 0x01);

        // ── Reset gesture direction counters ─────────────────────────────────
        resetCounts();

        // ── RTES: timerfd-driven I2C sampling ────────────────────────────────
        // The APDS-9960 INT pin is not wired; instead a timerfd fires every
        // POLL_INTERVAL_MS. The worker thread blocks on ::read(timerFd_) — a
        // true kernel sleep consuming zero CPU — then checks GSTATUS on wake
        // to confirm data validity before reading proximity / gesture FIFOs.
        timerFd_ = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerFd_ < 0)
            throw std::runtime_error("GestureSensor: timerfd_create failed");

        struct itimerspec its{};
        its.it_value.tv_sec     = 0;
        its.it_value.tv_nsec    = POLL_INTERVAL_MS * 1'000'000L;  // first fire
        its.it_interval.tv_sec  = 0;
        its.it_interval.tv_nsec = POLL_INTERVAL_MS * 1'000'000L;  // repeat
        if (timerfd_settime(timerFd_, 0, &its, nullptr) < 0)
            throw std::runtime_error("GestureSensor: timerfd_settime failed");

        running_ = true;
        workerThread_ = std::thread(&GestureSensor::worker, this);
        return true;
    } catch (const std::exception& e) {
        if (errorCallback_) errorCallback_(e.what());
        running_ = false;
        if (timerFd_ >= 0) {
            close(timerFd_);
            timerFd_ = -1;
        }
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
        return false;
    }
}

void GestureSensor::shutdown() {
    if (!running_) return;
    running_ = false;

    // Close timerfd first so the blocking read() in the worker unblocks
    if (timerFd_ >= 0) {
        close(timerFd_);
        timerFd_ = -1;
    }

    if (workerThread_.joinable()) {
        workerThread_.join();
    }

    if (fd_ >= 0) {
        try {
            // Power off safely without propagating exceptions from destruction
            writeRegister(APDS9960_ENABLE, 0x00);
        } catch (const std::exception& e) {
            Logger::error("GestureSensor shutdown exception swallowed: " + std::string(e.what()));
        }
        close(fd_);
        fd_ = -1;
    }
}

void GestureSensor::registerEventCallback(IProximitySensor::EventCallback cb) {
    eventCallback_ = std::move(cb);
}

void GestureSensor::registerErrorCallback(IProximitySensor::ErrorCallback cb) {
    errorCallback_ = std::move(cb);
}

#ifdef AQUAFLOW_TESTING
void GestureSensor::emitEventForTest(const GestureEvent& event) const {
    if (eventCallback_) {
        eventCallback_(event);
    }
}
#endif

// ── gestureValid() — Ported from Adafruit ────────────────────────────────────
// Checks GSTATUS register bit 0 (GVALID) to see if gesture FIFO has data.
bool GestureSensor::gestureValid() {
    uint8_t gstatus = readRegister(APDS9960_GSTATUS);
    return (gstatus & 0x01) != 0;
}

// ── resetCounts() — Ported from Adafruit ─────────────────────────────────────
void GestureSensor::resetCounts() {
    UCount_ = 0;
    DCount_ = 0;
    LCount_ = 0;
    RCount_ = 0;
}

// ── readGesture() — Ported from Adafruit_APDS9960::readGesture() ─────────────
// This is a NON-BLOCKING single-pass adaptation of the Adafruit algorithm.
// The original Arduino version loops with delay(30) + millis() timeout.
// Our version is called on each timerfd tick (every 50ms) so we do ONE pass
// per tick and let the counter state (UCount/DCount/LCount/RCount) persist
// across ticks to build up the gesture direction over time.
//
// Returns: gesture code (GESTURE_UP/DOWN/LEFT/RIGHT) or 0 if none yet.
// Reference: Adafruit_APDS9960_Library-1.3.1/Adafruit_APDS9960.cpp, lines 406-466
uint8_t GestureSensor::readGesture() {
    // ── Check for FIFO overflow (GFOV) before reading ────────────────────────
    // If the 32-dataset FIFO has overflowed, all data is corrupt.
    // Clear it and return — we'll catch the next gesture on the next tick.
    // Reference: CircuitPython apds9960.py gesture() overflow handling.
    {
        uint8_t gstatus = readRegister(APDS9960_GSTATUS);
        if (gstatus & 0x02) {  // GFOV = bit 1
            writeRegister(APDS9960_GCONF4, 0x04);  // GFIFO_CLR
            resetCounts();
            // Re-enable gesture mode (GFIFO_CLR auto-clears but resets GMODE)
            writeRegister(APDS9960_GCONF4, 0x01);  // GMODE=1
            return 0;
        }
    }

    if (!gestureValid()) {
        // No data available — check if we've timed out with pending counts
        if ((UCount_ > 0 || DCount_ > 0 || LCount_ > 0 || RCount_ > 0) &&
            gestureLastActivity_ > 0 && (nowMs() - gestureLastActivity_ > 300)) {
            resetCounts();
        }
        return 0;
    }

    // Read how many datasets are available in the FIFO
    uint8_t toRead = readRegister(APDS9960_GFLVL);
    if (toRead == 0) return 0;

    // ── Burst read FIFO data (single I2C transaction) ────────────────────────
    // Each dataset is 4 bytes: U, D, L, R.
    // We cap at 32 datasets (128 bytes) to match the hardware FIFO depth.
    std::array<uint8_t, 128> buf{};
    uint8_t bytesToRead = (toRead > 32) ? 128 : (toRead * 4);
    readBurst(APDS9960_GFIFO_U, buf.data(), bytesToRead);

    // ── Process ALL datasets in the batch ─────────────────────────────────────
    // The Adafruit Arduino library processes one dataset per loop iteration,
    // but it runs in a blocking while(1) loop. In our non-blocking adaptation
    // we get one call per 50ms tick, so we must process the entire FIFO batch
    // now — otherwise datasets are discarded and the counter state machine
    // never sees the direction reversal that constitutes a complete gesture.
    uint8_t gestureReceived = 0;
    int datasetsRead = bytesToRead / 4;

    for (int i = 0; i < datasetsRead; i++) {
        int offset = i * 4;
        int up_down_diff = 0;
        int left_right_diff = 0;

        if (std::abs((int)buf[offset] - (int)buf[offset + 1]) > 13)
            up_down_diff = (int)buf[offset] - (int)buf[offset + 1];

        if (std::abs((int)buf[offset + 2] - (int)buf[offset + 3]) > 13)
            left_right_diff = (int)buf[offset + 2] - (int)buf[offset + 3];

        // ── Direction state machine (ported from Adafruit) ───────────────────
        // A gesture is detected when the direction reverses: e.g. if DCount > 0
        // (hand was initially closer to the DOWN sensor) and now up_down_diff < 0
        // (hand moved toward UP sensor), that's an UP gesture.

        if (up_down_diff != 0) {
            if (up_down_diff < 0) {
                if (DCount_ > 0) {
                    gestureReceived = GESTURE_UP;
                } else {
                    UCount_++;
                }
            } else {
                if (UCount_ > 0) {
                    gestureReceived = GESTURE_DOWN;
                } else {
                    DCount_++;
                }
            }
        }

        if (left_right_diff != 0) {
            if (left_right_diff < 0) {
                if (RCount_ > 0) {
                    gestureReceived = GESTURE_LEFT;
                } else {
                    LCount_++;
                }
            } else {
                if (LCount_ > 0) {
                    gestureReceived = GESTURE_RIGHT;
                } else {
                    RCount_++;
                }
            }
        }

        // Update activity timestamp if we saw any non-zero diff
        if (up_down_diff != 0 || left_right_diff != 0) {
            gestureLastActivity_ = nowMs();
        }

        // If a completed gesture was found in this dataset, return immediately
        if (gestureReceived != 0) {
            resetCounts();
            return gestureReceived;
        }
    }

    // No completed gesture yet — check for timeout
    if (gestureLastActivity_ > 0 && (nowMs() - gestureLastActivity_ > 300)) {
        resetCounts();
    }

    return 0;
}

// ── worker() — Polling loop ──────────────────────────────────────────────────
void GestureSensor::worker() {
    bool isTriggered = false;

    while (running_) {
        try {
            // 1. Evaluate Proximity
            int prox = readRegister(APDS9960_PDATA);
            bool currentlyTriggered = (prox > threshold_);

            if (currentlyTriggered && !isTriggered) {
                isTriggered = true;
                if (eventCallback_) {
                    eventCallback_({ProximityState::PROXIMITY_TRIGGERED,
                                    GestureDir::NONE,
                                    prox,
                                    {static_cast<float>(prox)}});
                }
            } else if (!currentlyTriggered && isTriggered) {
                int hysteresis_lower = (threshold_ - 20 >= 0) ? (threshold_ - 20) : 0;
                if (prox < hysteresis_lower) {
                    isTriggered = false;
                    if (eventCallback_) {
                        eventCallback_({ProximityState::PROXIMITY_CLEARED,
                                        GestureDir::NONE,
                                        prox,
                                        {static_cast<float>(prox)}});
                    }
                }
            }

            // 2. Evaluate Gesture (Adafruit algorithm)
            uint8_t gesture = readGesture();
            if (gesture != 0 && eventCallback_) {
                GestureDir dir = GestureDir::NONE;
                switch (gesture) {
                    case GESTURE_UP:    dir = GestureDir::UP;    break;
                    case GESTURE_DOWN:  dir = GestureDir::DOWN;  break;
                    case GESTURE_LEFT:  dir = GestureDir::LEFT;  break;
                    case GESTURE_RIGHT: dir = GestureDir::RIGHT; break;
                }
                if (dir != GestureDir::NONE) {
                    eventCallback_({ProximityState::NONE,
                                    dir,
                                    prox,
                                    {static_cast<float>(prox)}});
                }
            }

        } catch (const std::exception& e) {
            if (errorCallback_) errorCallback_(e.what());
        }

        // ── RTES-compliant blocking wait (timerfd) ───────────────────────────
        // Thread blocks here until the kernel fires the timerfd at the next
        // POLL_INTERVAL_MS boundary — zero CPU while idle, no sleep() call.
        // On wake, I2C registers are read and GSTATUS checked for data validity.
        // If timerFd_ was closed by shutdown(), read() returns -1 → loop exits.
        uint64_t expirations = 0;
        if (::read(timerFd_, &expirations, sizeof(expirations)) < 0) {
            break;  // timerfd closed by shutdown() — clean exit
        }
    }
}

// ── I2C Helpers ──────────────────────────────────────────────────────────────

uint8_t GestureSensor::readRegister(uint8_t reg) {
    if (::write(fd_, &reg, 1) != 1) {
        throw std::runtime_error("Failed to write register address");
    }
    uint8_t val;
    if (::read(fd_, &val, 1) != 1) {
        throw std::runtime_error("Failed to read register value");
    }
    return val;
}

void GestureSensor::writeRegister(uint8_t reg, uint8_t value) {
    std::array<uint8_t, 2> buf = {reg, value};
    if (::write(fd_, buf.data(), 2) != 2) {
        throw std::runtime_error("Failed to write to register");
    }
}

// ── readBurst() — Multi-byte I2C read (single transaction) ───────────────────
// Equivalent to Adafruit_APDS9960::read(reg, buf, num) which calls
// i2c_dev->write_then_read(). On Linux I2C, we write the register address
// then read len bytes in a single system call sequence.
// This is critical for the gesture FIFO: all 4 bytes (U/D/L/R) per dataset
// must be read in one burst or the FIFO pointer advances incorrectly.
void GestureSensor::readBurst(uint8_t reg, uint8_t* buf, uint8_t len) {
    if (::write(fd_, &reg, 1) != 1) {
        throw std::runtime_error("readBurst: failed to write register address");
    }
    if (::read(fd_, buf, len) != len) {
        throw std::runtime_error("readBurst: failed to read burst data");
    }
}

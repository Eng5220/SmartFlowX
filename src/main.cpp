#include "PinConfig.h"
#include "app/AquaFlowApp.h"
#include "hardware/GestureSensor.h"
#include "hardware/PumpController.h"
#include "hardware/FlowMeter.h"
#include "hardware/LcdDisplay.h"
#include "utils/Logger.h"

#include <csignal>
#include <signal.h>

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    // ── Construct hardware drivers ────────────────────────────────────────────
    GestureSensor    gestureSensor(GESTURE_I2C_BUS, GESTURE_I2C_ADDR, GESTURE_THRESHOLD);
    PumpController   pump(GPIO_CHIP_NO, PUMP_PIN);
    FlowMeter        flowMeter(GPIO_CHIP_NO, FLOW_PIN, static_cast<float>(ML_PER_PULSE));
    LcdDisplay       lcd(LCD_I2C_BUS, LCD_I2C_ADDRESS);

    // ── Initialise hardware ───────────────────────────────────────────────────
    if (!gestureSensor.init()) {
        Logger::error("Failed to initialise GestureSensor");
        return 1;
    }
    if (!pump.init()) {
        Logger::error("Failed to initialise PumpController");
        gestureSensor.shutdown();
        return 1;
    }
    if (!flowMeter.init()) {
        Logger::error("Failed to initialise FlowMeter");
        pump.shutdown();
        gestureSensor.shutdown();
        return 1;
    }
    if (!lcd.init()) {
        Logger::error("Failed to initialise LcdDisplay — continuing without display");
    }

    // ── Application Orchestration ─────────────────────────────────────────────
    AquaFlowApp app(gestureSensor, pump, flowMeter, lcd);
    
    Logger::info("=== AquaFlow Filling Machine Started ===");
    Logger::info("Hold time     : " + std::to_string(HOLD_TIME_SECONDS) + " s");
    Logger::info("Target volume : " + std::to_string(TARGET_VOLUME_ML) + " ml");
    Logger::info("Gesture sensor: I2C bus " + std::to_string(GESTURE_I2C_BUS) + ", addr 0x39");

    app.start();

    // ── Block main thread until SIGINT (Ctrl+C) via sigwait ──────────────────
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

    int sig = 0;
    sigwait(&sigset, &sig);
    Logger::info("Signal " + std::to_string(sig) + " received — shutting down.");

    // ── Shutdown ─────────────────────────────────────────────────────────────
    app.shutdown();

    lcd.shutdown();
    flowMeter.shutdown();
    pump.shutdown();
    gestureSensor.shutdown();

    Logger::info("AquaFlow stopped. Goodbye.");
    return 0;
}

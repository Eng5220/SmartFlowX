#include "PinConfig.h"
#include "hardware/PumpController.h"
#include "hardware/FlowMeter.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <thread>

// ─── Graceful shutdown on Ctrl+C ─────────────────────────────────────────────
int main() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

    auto waitForStop = [&sigset](std::chrono::milliseconds timeout) {
        timespec ts{};
        ts.tv_sec = timeout.count() / 1000;
        ts.tv_nsec = static_cast<long>((timeout.count() % 1000) * 1000000);
        int sig = sigtimedwait(&sigset, nullptr, &ts);
        if (sig == SIGINT || sig == SIGTERM) {
            std::cout << "\n[Signal " << sig << "] Stopping pump...\n";
            return true;
        }
        return false;
    };

    std::cout << "=== Pump + Flow Meter Calibration Test ===\n";
    std::cout << "Current ml/pulse : " << ML_PER_PULSE << "\n";
    std::cout << "Press Ctrl+C when your beaker hits the target volume.\n\n";

    PumpController pump(GPIO_CHIP_NO, PUMP_PIN);
    FlowMeter      flow(GPIO_CHIP_NO, FLOW_PIN, static_cast<float>(ML_PER_PULSE));

    if (!pump.init()) { std::cerr << "ERROR: Pump init failed\n"; return 1; }
    if (!flow.init()) { std::cerr << "ERROR: FlowMeter init failed\n"; pump.shutdown(); return 1; }

    flow.resetCount();
    pump.turnOn();
    std::cout << "Pump ON...\n\n";

    // Run until Ctrl+C — no auto-stop
    while (true) {
        if (waitForStop(std::chrono::milliseconds(500))) {
            break;
        }

        int    pulses = flow.getPulseCount();
        double vol    = flow.getVolumeML();

        std::cout << "  Pulses: " << std::setw(6) << pulses
                  << "   Vol (estimated): " << std::fixed << std::setprecision(1)
                  << std::setw(7) << vol << " ml\n";
    }

    pump.turnOff();

    int finalPulses = flow.getPulseCount();
    std::cout << "\nPump OFF.\n";
    std::cout << "Sensor counted : " << finalPulses << " pulses\n\n";

    // ── Calibration helper ────────────────────────────────────────────────────
    std::cout << "How many ml did you actually measure in the beaker? ";
    double actualMl = 0.0;
    std::cin >> actualMl;

    if (finalPulses > 0 && actualMl > 0.0) {
        double newMlPerPulse = actualMl / static_cast<double>(finalPulses);
        std::cout << "\n── Calibration Result ───────────────────────────────\n";
        std::cout << "  Actual volume  : " << std::fixed << std::setprecision(1) << actualMl    << " ml\n";
        std::cout << "  Pulse count    : " << finalPulses << "\n";
        std::cout << "  Old ml/pulse   : " << ML_PER_PULSE << "\n";
        std::cout << "  NEW ml/pulse   : " << std::setprecision(6) << newMlPerPulse << "\n";
        std::cout << "\nUpdate PinConfig.h:\n";
        std::cout << "  constexpr double ML_PER_PULSE = "
                  << std::setprecision(6) << newMlPerPulse << ";\n";
        std::cout << "─────────────────────────────────────────────────────\n";
    } else {
        std::cout << "Invalid input — calibration skipped.\n";
    }

    flow.shutdown();
    pump.shutdown();
    return 0;
}

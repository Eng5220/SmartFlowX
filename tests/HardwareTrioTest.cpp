#include "PinConfig.h"
#include "hardware/PumpController.h"
#include "hardware/FlowMeter.h"
#include "hardware/GestureSensor.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <iomanip>
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
            std::cout << "\n[Signal " << sig << "] Stopping...\n";
            return true;
        }
        return false;
    };

    std::cout << "=== Touchless Dispenser Gesture Terminal Test ===\n";
    std::cout << "ML per pulse  : " << ML_PER_PULSE << "\n\n";

    // ── Construct ─────────────────────────────────────────────────────────────
    PumpController pump(GPIO_CHIP_NO, PUMP_PIN);
    FlowMeter      flow(GPIO_CHIP_NO, FLOW_PIN, static_cast<float>(ML_PER_PULSE));
    GestureSensor  gesture; // defaults to bus 1, addr 0x39, thresh 100

    // ── Initialise ────────────────────────────────────────────────────────────
    if (!pump.init()) {
        std::cerr << "ERROR: PumpController failed to init\n"; return 1;
    }
    if (!flow.init()) {
        std::cerr << "ERROR: FlowMeter failed to init\n";
        pump.shutdown(); return 1;
    }
    if (!gesture.init()) {
        std::cerr << "ERROR: GestureSensor failed to init\n";
        pump.shutdown(); flow.shutdown(); return 1;
    }

    enum class AppState { SELECTING, CONFIRMED, DISPENSING, DONE };
    std::atomic<AppState> appState(AppState::SELECTING);

    // McDonalds UK Sizes!
    std::atomic<int> activeTargetVolume(400); // Default to Medium
    bool waitingMessagePrinted = false;

    gesture.registerEventCallback([&](const GestureEvent& ev) {
        
        // 1. Gesture Size Selection Logic
        if (appState == AppState::SELECTING) {
            if (ev.getDirection() == GestureDir::LEFT) {
                if (activeTargetVolume == 500) activeTargetVolume = 400; // L -> M
                else if (activeTargetVolume == 400) activeTargetVolume = 250; // M -> S
                
                std::cout << "\n<<< SWIPE LEFT: Size changed to ";
                if (activeTargetVolume == 250) std::cout << "SMALL (250ml)\n";
                else std::cout << "MEDIUM (400ml)\n";
                waitingMessagePrinted = false;
            } 
            else if (ev.getDirection() == GestureDir::RIGHT) {
                if (activeTargetVolume == 250) activeTargetVolume = 400; // S -> M
                else if (activeTargetVolume == 400) activeTargetVolume = 500; // M -> L
                
                std::cout << "\n>>> SWIPE RIGHT: Size changed to ";
                if (activeTargetVolume == 500) std::cout << "LARGE (500ml)\n";
                else std::cout << "MEDIUM (400ml)\n";
                waitingMessagePrinted = false;
            }
            else if (ev.getDirection() == GestureDir::UP || ev.getDirection() == GestureDir::DOWN) {
                // Confirm selection with vertical swipe!
                std::cout << "\n*** SIZE CONFIRMED: " << activeTargetVolume << "ml ***\n";
                std::cout << "[-] Place your cup under the nozzle to dispense.\n";
                appState = AppState::CONFIRMED;
                waitingMessagePrinted = false;
            }
        }
        
        // 2. Proximity Dispensing Logic (Only triggers if confirmed!)
        else if (appState == AppState::CONFIRMED) {
            if (ev.getState() == ProximityState::PROXIMITY_TRIGGERED) {
                std::cout << "\n>>> CUP DETECTED! Dispensing " << activeTargetVolume << " ml.\n";
                flow.resetCount();
                appState = AppState::DISPENSING;
                waitingMessagePrinted = false;
                pump.turnOn();
            }
        } 
        
        // 3. Emergency Stop Logic
        else if (appState == AppState::DISPENSING) {
            if (ev.getState() == ProximityState::PROXIMITY_CLEARED) {
                std::cout << "\n>>> CUP REMOVED EARLY! Dispensing aborted. Returning to selection screen.\n";
                pump.turnOff();
                appState = AppState::SELECTING; 
                waitingMessagePrinted = false;
            }
        }
        
        // 4. Finished Logic
        else if (appState == AppState::DONE) {
            if (ev.getState() == ProximityState::PROXIMITY_CLEARED) {
                std::cout << "\n[-] Cup removed. Returning to size selection mode.\n";
                appState = AppState::SELECTING;
                waitingMessagePrinted = false;
            }
        }
    });

    std::cout << "\n[-] System idle. Swipe Left/Right to select size.\n";
    std::cout << "[-] Swipe UP or DOWN to confirm selection.\n";
    std::cout << "[-] Current Size: MEDIUM (400ml).\n";

    // ── Live volume loop ──────────────────────────────────────────────────────
    while (true) {
        if (waitForStop(std::chrono::milliseconds(200))) {
            break;
        }

        if (appState == AppState::DISPENSING) {
            double vol = flow.getVolumeML();
            int pulses = flow.getPulseCount();

            std::cout << "\r  [Pump RUNNING] Pulses: " << std::setw(5) << pulses
                      << "   Vol: " << std::fixed << std::setprecision(1)
                      << std::setw(7) << vol << " ml"
                      << std::flush;

            // Stop when we hit the dynamically selected size
            if (flow.getVolumeML() >= static_cast<double>(activeTargetVolume)) {
                std::cout << "\n\n*** TARGET " << activeTargetVolume << "ml REACHED! ***\n";
                pump.turnOff();
                appState = AppState::DONE;
                waitingMessagePrinted = false;
            }
        } else if (appState == AppState::DONE) {
            if (!waitingMessagePrinted) {
                std::cout << "\n[-] Please remove your full cup.\n";
                waitingMessagePrinted = true;
            }
        } else if (appState == AppState::SELECTING) {
            // Idle but no target reached and not dispensing
            if (!waitingMessagePrinted) {
                std::cout << "\n[-] Awaiting Gesture... (L/R to change, U/D to confirm)\n";
                waitingMessagePrinted = true;
            }
        }
    }

cleanup:
    pump.turnOff();
    pump.shutdown();
    flow.shutdown();
    gesture.shutdown();

    std::cout << "\nTest complete.\n";
    return 0;
}

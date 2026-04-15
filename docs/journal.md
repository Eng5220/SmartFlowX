# Developer Journal

A chronological log of work done in the lab, daily goals, and simple tasks.

---

## 20 March 2026
**Time:** 14:16
**Location:** Rankine Lab 509

**Goals:**
- Identify the correct water pump and replace the solenoid valve setup.
- Figure out how to safely connect the pump to the Raspberry Pi.
- Resolve 5V power distribution across the various sensors and the pump.

**Notes:**
- We decided to switch back from the 12V Solenoid valve to a submersible pump because the solenoid was bulky and not performing as expected with the relay setup.
- Explored powering the pump. Tried direct pi connection (Bad Idea). Explored 3x 1.5v AA batteries (didn't go this route). Ultimately wired a T1P121 transistor with a flyback diode and a 1000 ohm resistor. 

---

## [Date TBD] 
**Time:** 11:23
**Location:** RTES lab

**Tasks:**
- [x] Connect the gesture sensor and have it working. (Read APDS-9960 specs at 11:26)
- [ ] Make a social media plan. We need to upload 5 times and get people to like.
- [ ] Research about the 3D printing.

**11:33 Subtask:**
- [ ] Find out how to connect the gesture sensor to the Raspberry Pi.

**Notes & Observations:**
- The wires we use are quite short and in some cases the connections are weak.
- Right now this is no big deal because we are testing our connections separately, but this will become important when we are bringing the product together.

---

## 14 April 2026

**Single Responsibility Review and Refactors**

### PumpController
- Responsibility: owns pump GPIO lifecycle and ON/OFF actuation.
- SRP decision: PASS.
- Changes made:
  - Replaced direct `std::cout` and `std::cerr` calls with `Logger::info()` and `Logger::error()`.
  - Replaced internal static relay flag with constructor-injected `DriveMode` (`RELAY_ACTIVE_LOW` or `TRANSISTOR_ACTIVE_HIGH`).
  - Default mode is `TRANSISTOR_ACTIVE_HIGH` to preserve current behaviour.

### FlowMeter
- Responsibility: detects flow pulses, debounces hardware noise, and converts pulses to millilitres.
- SRP decision: PASS.
- Changes made:
  - Removed `hasReachedTarget()` from `FlowMeter` to avoid business-rule leakage.
  - Kept `FlowMeter` as a measurement driver exposing `getPulseCount()` and `getVolumeML()` only.
  - Moved target-comparison logic into controllers/callers.

### LcdDisplay
- Responsibility: low-level 16x2 LCD rendering over I2C.
- SRP decision: PASS.
- Changes made:
  - Removed business-formatting methods (`showVolume()` and `showStatus()`).
  - Moved row text construction (`Vol:`, `Bottles:`) to `main.cpp` orchestrator logic.
  - Routed LCD init/shutdown logs through `Logger`.
  - Kept shutdown as hardware-only (backlight off and fd close); farewell text is now composed by caller before shutdown.

### GestureSensor
- Responsibility: read APDS9960 registers and emit proximity/gesture events.
- SRP decision: PASS.
- Changes made:
  - Replaced direct warning output with `Logger::warn()` for unknown device ID path.

### Monitor
- Responsibility: format and log state updates and transitions.
- SRP decision: PASS.
- Changes made:
  - Removed unused `registerCallback` API and internal callback member (YAGNI cleanup).
  - Assumed full responsibility for format and emission of human-readable state transition logs, making `FillingController` independent of UI/logging.

### FillingController
- Responsibility: Orchestrate hardware components for the filling cycle.
- SRP decision: PASS.
- Changes made:
  - Removed all `Logger` dependencies. All transition logs are deferred to `Monitor` for ultra-pure SRP.

### Final SRP Outcome
- All core classes now have one clear reason to change.
- Business rules and UI text formatting are kept in orchestration/presentation layers, not in hardware drivers.

### Open/Closed Principle Review (14 April 2026)
- `IHardwareDevice` remains the lifecycle extension point (`init()` and `shutdown()`), and `PumpController`, `FlowMeter`, `GestureSensor`, and `LcdDisplay` extend it without modifying the base class.
- Added role interfaces: `IProximitySensor`, `IPump`, and `IFlowMeter`.
- Refactored `FillingController` to depend only on those abstractions, not concrete hardware drivers.
- Evidence added: updated class diagram and hierarchy documentation in `docs/architecture.md`.
- Verdict: OCP/DIP pass at both hardware and orchestration layers.

---

**Time:** 17:31
**Location:** Local workspace / Codex review pass

**Goals:**
- Improve the code in ways that are directly supported by the RTES course material.
- Strengthen encapsulation and unit-testing quality without weakening the event-driven runtime design.
- Bring hardware-driver logging into line with the documented SRP decisions.

**Tasks Completed:**
- [x] Removed `#define private public` from the unit tests.
- [x] Added test-only seams behind `AQUAFLOW_TESTING` so tests can inject safe synthetic state without touching private members.
- [x] Reworked `FlowMeterTest`, `FillingControllerTest`, and `PumpControllerTest` to avoid real hardware initialisation.
- [x] Removed the real-time `sleep_for()` delay from `FillingControllerTest`.
- [x] Routed remaining hardware-driver console output through `Logger`.
- [x] Added null-safe handling in `PumpController` so unit-test simulation mode does not dereference a missing GPIO request.
- [x] Enabled `AQUAFLOW_TESTING` only for the `filling_tests` target in CMake.

**Notes & Rationale:**
- The main driver for these changes was the course requirement around **encapsulation**. The checklist says top-band work should expose a clear public interface with data accessed through getters, setters, or callbacks only. Our previous tests were violating this by redefining `private` as `public` and directly mutating internal fields like `pulseCount_` and `eventCallback_`.
- To fix this without weakening the production design, we added **test-only seams** guarded by `#ifdef AQUAFLOW_TESTING`. This means the real application binary keeps its original encapsulation, while the unit tests get small, explicit hooks for simulation.
- `FlowMeter` now has `injectPulseCountForTest(int)` so the test can verify volume calculations and target detection without touching private atomics directly.
- `GestureSensor` now has `emitEventForTest(const GestureEvent&)` so the controller tests can simulate proximity events through a method instead of reaching into the stored callback function.
- `FillingController` now has `forceHoldElapsedForTest(double)` so we can fast-forward the confirmation phase in tests without sleeping for 2.1 seconds. This makes the tests deterministic and avoids time-based flakiness.
- `PumpController` now has `enableSimulationForTest()` so the pump tests can check logical ON/OFF behaviour without requiring `/dev/gpiochip...` to exist. We also wrapped `request_->set_value(...)` in null checks so simulation mode is safe.
- These changes are course-backed because the marking material explicitly values **unit testing**, **encapsulation**, and **safe data access**, and our previous tests were undermining that story even though the production architecture was strong.

**Logger / SRP Cleanup:**
- `GestureSensor.cpp` was still using `std::cerr` directly for the unknown APDS-9960 device ID warning.
- `LcdDisplay.cpp` was still using `std::cout` / `std::cerr` directly for init/shutdown messages.
- We replaced those direct console writes with `Logger::warn`, `Logger::info`, and `Logger::error` so the hardware drivers remain focused on hardware responsibilities only.
- This matches the ADR decision that hardware-driver output should be routed through `Logger` to better respect **Single Responsibility Principle (SRP)** and keep console formatting/thread-safe output centralized.

**Files Changed:**
- `src/hardware/FlowMeter.h`
- `src/hardware/FlowMeter.cpp`
- `src/hardware/GestureSensor.h`
- `src/hardware/GestureSensor.cpp`
- `src/hardware/LcdDisplay.cpp`
- `src/hardware/PumpController.h`
- `src/hardware/PumpController.cpp`
- `src/state/FillingController.h`
- `src/state/FillingController.cpp`
- `tests/FlowMeterTest.cpp`
- `tests/FillingControllerTest.cpp`
- `tests/PumpControllerTest.cpp`
- `tests/CMakeLists.txt`

**Outcome:**
- The runtime architecture is still event-driven: `FlowMeter` still waits on GPIO edge events and `GestureSensor` still uses timerfd-based blocking behavior.
- The unit tests are now much closer to true unit tests instead of semi-integration tests.
- The project documentation and the actual code are now more consistent with each other, especially around encapsulation, SRP, and testability.

**Limitations / Follow-up:**
- I could not run `cmake` in the current shell because `cmake` was not installed or not available on `PATH`, so the build/test pass still needs to be verified in a configured development environment.
- A sensible next improvement would be to harden `GestureSensor::init()` so failure paths are more transactional and leave the object in a cleaner state if setup fails partway through.

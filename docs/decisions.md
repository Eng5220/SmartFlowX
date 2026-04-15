# Architecture & Technical Decisions (ADRs)

A master log of technical bottlenecks and the resolutions implemented for the AquaFlow project.

---

## [CLOSED] Decision 1: Pump vs Solenoid Valve
- **Problem:** The original setup utilised a 12V Solenoid valve. It looked solid and we bought a relay for it, but ultimately we couldn't get it working with our setup and it made the footprint too bulky.
- **Resolution:** Reverted to a DC submersible pump (JT80SL) instead of the solenoid valve.

## Decision 2: Managing Pump Power Draw
- **Problem:** The JT80SL draws up to 220mA, which is > 10x what a single Raspberry Pi GPIO pin can safely handle (16 mA max). 
- **Resolution:** We cannot connect it directly. Instead of using 3x 1.5v AA batteries, we wired a **T1P121 transistor with a flyback diode and a 1000 ohm resistor**. (Note: Consider switching to a MOSFET in the future).
- *Open Concern:* We need to establish if the concern about the 5V pin being over-occupied is a real concern and how to solve it.

## Decision 3: Resolving 5V Rail Contention
- **Problem:** We have 2 sensors, a pump, and LCDs, and they are all competing for the Pi's 5V rail. We used a relay module for the pump that took up two 5V pins.
- **Resolution Plan:** The pump should ideally be powered from a separate 5V supply rather than directly from the Pi. This avoids startup current spikes that can drop voltage and destabilise the Pi. Pi GPIO should only be used for control.
- **Proposed setup:** 
  - External 5V supply for pump.
  - Relay or MOSFET for switching.
  - Common ground shared with Raspberry Pi.
  *(Note: We already have a 12V power supply and can use an LM2596 Buck Converter).*

## Decision 10: Fluid Dynamics & Tubing Layout

- **Context:** We have 1m of 6mm tubing and need to determine the optimal configuration for connecting the submersible pump, the YF-S401 water flow sensor, and the dispensing nozzle.
- **Decision:** **Use the shortest possible tubing run. The pump must be submerged, with the flow sensor placed inline after the pump.**
- **Rationale for Tubing Length (Shorter is Better):**
  - **Priming Latency:** A longer tube holds more air when empty, causing a noticeable delay before water reaches the cup when the pump starts. Shorter tubes mean a snappier, more responsive system.
  - **Head Loss:** Longer tubes create more flow friction, putting unnecessary strain on our small 5V DC pump and reducing the maximum flow rate.
  - **After-Drip:** A long tube acts as a reservoir. Once the pump cuts off, any water remaining in the tube will slowly drip out, which completely ruins the precision of our calibrated dispensing volumes.
- **Rationale for Component Positioning:**
  - **Pump Submersion:** The JT80SL is a *submersible* pump. Because it draws water through a slotted casing rather than an inlet port, there is no way around it: it **must** sit fully submerged at the bottom of the water source/reservoir.
  - **Flow Sensor Placement:** The flow sensor must be placed on the pressure-side (attached directly to the tubing coming *out* of the pump). It requires positive, pressurized flow to spin its internal hall-effect turbine accurately. 
  - **Proximity:** Keep the flow sensor relatively close to the pump but try to allow a straight run of tubing leading into it. Minimizing curves right before the sensor reduces turbulent water flow, leading to much more stable and predictable pulse readings.

---

## Decision 4: Hardware Scope - Keep Current Design, Do Not Expand

- **Context:** Proposal raised to extend the hardware (viscosity sensing, restoring the ultrasonic sensor). The counter-argument is that expanding hardware scope at this stage risks diverting time away from code quality - the primary marking criterion.
- **Decision:** **Freeze the hardware at its current scope.** The system already fulfils its core function (touchless cup detection → pump → precise volume dispensing) and does so well. Adding components for novelty at this stage would provide diminishing returns.
- **Rationale:**
  - The assessment is marked predominantly on code quality (SOLID, encapsulation, real-time architecture, revision control, documentation). Hardware is not a graded criterion on its own.
  - The novelty angle was assessed at the presentation stage, which has already passed.
  - Bringing back the ultrasonic sensor is redundant - the APDS-9960 gesture sensor handles proximity detection and is already integrated. Running both in parallel would add complexity with no functional benefit.
  - Time is better invested in: unit tests, documentation, UML diagrams, and commit quality - all of which directly affect the final grade.

---

## Decision 5: Pump Power Supply - Evaluate Powerbank Integration

- **Context:** Current setup drives the pump via a TIP122 transistor + flyback diode from the Pi's 5V rail. A portable powerbank is available that could act as a dedicated external supply.
- **Decision:** **Conditionally adopt the powerbank if integration is non-invasive.** If it can be wired as a simple external 5V supply (common ground with Pi, pump powered via powerbank, transistor control signal still from GPIO) without requiring a redesign of the existing circuit, it is worth adopting.
- **Rationale:**
  - A dedicated supply eliminates current spikes on the Pi's 5V rail, reducing the risk of the Pi browning out mid-dispense.
  - The transistor switching circuit (base resistor + flyback diode) remains unchanged - only the supply rail moves.
  - If it requires more than 30 minutes of hardware rework, defer. Code quality is the priority.
- **Open action:** Wire powerbank → collector side of TIP122; verify common ground with Pi. Test pump performance under load.

---

## Decision 6: Volume Measurement - Retain Flow Meter, Improve Calibration

- **Context:** Proposal to replace the YF-S401 flow meter with a load cell for more accurate volume measurement. The flow meter required field calibration against a beaker.
- **Decision:** **Retain the YF-S401 flow meter.** Recalibrate using a more accurate volumetric flask if precision is still insufficient.
- **Rationale:**
  - Replacing the flow meter with a load cell would require a new hardware driver class, new calibration, and re-integration with `FillingController` - a significant diversion of time.
  - The flow meter's event-driven pulse counting is already a strong RTES demonstration (GPIO interrupt → `atomic<int>` → callback). A load cell would likely require polling an ADC, which is architecturally weaker for this course.
  - The current calibration (`0.073774 ml/pulse`, derived from 2711 pulses for 200 ml) is documented and reproducible. A more accurate flask would tighten this without touching code.
  - A load cell is only justified if the flow meter's accuracy is proven to be a functional problem during the demo. Until then, it is not worth the risk.

---

## Decision 7: Gesture Sensor - Improve C++ Implementation, Do Not Switch Libraries

- **Context:** Gesture direction detection (swipe UP/DOWN/LEFT/RIGHT) works reliably in Python but produces unreliable results in C++. Proximity detection works well in both. Proposal raised to port the Python library logic to C++.
- **Decision:** **Improve the existing C++ FIFO parsing implementation.** The full APDS-9960 register-level documentation is available; the Python library is open-source and can be used as a reference for the algorithm, not as a dependency.
- **Rationale:**
  - The Python library (e.g., `adafruit-circuitpython-apds9960`) is not a C++ dependency and cannot be called directly. "Converting" it means reading its gesture-decoding logic and reimplementing it in C++ - which is exactly what our current `GestureSensor.cpp` does.
  - The likely cause of the discrepancy is the FIFO read timing. Python libraries typically add small delays between FIFO reads to allow the gesture engine to settle; our C++ implementation polls the FIFO on a 50 ms timerfd tick which may be too coarse or too fast for certain swipe speeds.
  - **Fix approach:** Compare the Python library's gesture-decoding thresholds and FIFO drain strategy against our current implementation. Tune `POLL_INTERVAL_MS`, gesture entry/exit thresholds (`GPENTH`/`GEXTH`), and the minimum-frame filter (`gesture_dataset_count_ > 4`).
  - If gestures are improved, this becomes a strong novelty differentiator for the demo. If they remain unreliable, proximity-only mode (cup detection) is sufficient for the core dispensing function.
- **On the ultrasonic sensor suggestion:** Rejected. An ultrasonic sensor cannot detect gesture direction and would only replicate the proximity function already handled by the APDS-9960.

---

## Decision 8: Project Complexity - Sufficient for Assessment Purposes

- **Context:** Concern raised that the project may be too simple from a hardware perspective relative to other teams.
- **Decision:** **Scope is adequate; shift focus to code depth and documentation quality.**
- **Rationale:**
  - Complexity in this course is judged on the *code*, not the number of hardware components. A simple hardware setup with excellent SOLID architecture, event-driven threading, comprehensive unit tests, and clean documentation will outperform a complex hardware setup with poor code quality.
  - AquaFlow already demonstrates: multi-threaded event-driven architecture (`GestureSensor` + `FlowMeter` + `Timer` each on independent worker threads), a proper state machine (`FillingController`), RAII memory management, and `libgpiod` v2 GPIO - all of which are exactly what the marking rubric rewards.
  - If gesture detection is polished and working for the demo, that alone is a compelling novelty angle that requires no additional hardware.

---

## Decision 9: Final Form Factor - Prototype HAT / Stripboard vs Custom PCB

- **Context:** The current breadboard prototype is messy with loose wires. A proposal was made to design and order a custom PCB for a cleaner final product. However, there is concern about the time investment required.
- **Decision:** **Migrate to a solderable Stripboard or Raspberry Pi Prototype HAT instead of a custom PCB.**
- **Rationale:**
  - Standard PCB fabrication and shipping lead times (e.g., JLCPCB) take 1-2 weeks, which introduces massive timeline risk if the board is delayed or requires a second revision.
  - Designing a PCB in KiCad diverts significant time away from code architecture, testing, and documentation - all of which carry heavy weight in the assessment marking criteria.
  - A simple perfboard, stripboard, or Pi Prototype HAT achieves the exact same goal (eliminating loose breadboard jumper wires, creating a robust physical unit for demo day) but can be soldered together in a single afternoon using standard lab equipment.
  - The circuit is incredibly simple (one TIP122, a diode, a single resistor, and standard female header pins for the sensors) making a full custom PCB functionally overkill.

---

## Decision 11: Route Hardware Output through Logger (SRP Enforcement)

- **Context:** The `PumpController` class used explicit `std::cout` and `std::cerr` calls for debugging and status updates (e.g., pump initialization, start/stop events).
- **Decision:** **Substitute `std::cout/cerr` with `Logger::info()/error()` within all hardware driver classes (starting with `PumpController`).**
- **Rationale:**
  - **Single Responsibility Principle (SRP):** A hardware driver class should purely manage the hardware (e.g., pulling a GPIO pin high/low). Handling user presentation, including console output formatting, violates SRP because it gives the class two reasons to change: 1) if the hardware swaps, or 2) if logging/UI needs alteration.
  - By routing messages through the `Logger` component, `PumpController` delegates the responsibility of formatting, routing, and thread-safety of logs back to a centralized utility designed for that purpose.

---

## Decision 12: Dependency Inversion for FillingController via Behavioral Interfaces

- **Context:** `FillingController` previously depended on concrete classes (`GestureSensor`, `PumpController`, `FlowMeter`) to call behavior methods (`registerEventCallback`, `turnOn`, `getVolumeML`). This limited Open/Closed compliance at the orchestration layer.
- **Decision:** **Introduce focused behavioral interfaces** in `include/`:
  - `IProximitySensor`
  - `IPump`
  - `IFlowMeter`
  and make `FillingController` depend on these abstractions only.
- **Rationale:**
  - This removes concrete driver coupling from high-level logic and improves OCP/DIP evidence for assessment.
  - It keeps interface count minimal (no broad interface explosion), while still covering domain behavior needed by the state machine.
  - We considered a template-based static polymorphism variant for zero virtual dispatch overhead, but chose runtime interfaces for maintainability and clearer design communication. At a 100 ms control period, virtual dispatch overhead is not operationally significant.
- **Applied Implementations:**
  1. **Dependency Inversion + Liskov Substitution:** The state machine (`FillingController`) accesses hardware strictly through the abstract `IHardwareDevice` interfaces, allowing real drivers to be freely swapped for test mocks without altering controller logic.
  2. **Open-Closed Principle:** Display outputs are decoupled entirely via asynchronous **Observer Callbacks**. The controller broadcasts events (`onStateChange`) via `Monitor`, keeping UI open for extension without touching core logic.
  3. **Event-Driven Threading:** `libgpiod` Edge Event workers and `sys/timerfd.h` isolate independent threads. The system gains robust real-time sub-millisecond execution, with thread safety managed via `std::atomic<bool>` guards.

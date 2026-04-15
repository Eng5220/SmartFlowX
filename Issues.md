# Session Notes - Water Dispenser Integration
**Date:** 20 March 2026, 14:16  
**Location:** Rankine Lab 509

---

## Session Goal

Get the pump, water flow sensor, ultrasonic sensor, and LCD all working together as a unified water dispenser — with the pump drawing water from the bottle into the cup automatically.

---

## Background: Why We Dropped the Solenoid Valve

We originally tried a 12V solenoid valve with a relay. It needed its own power supply, turned out to be bulky, and we couldn't get it working reliably with our setup. Reverting to a pump is the cleaner call.

---

## Step 1 — Confirm Pump Model

**SGerste JT80SL DC 3–6V Submersible Mini Water Pump**

| Parameter | Value |
|---|---|
| Voltage range | DC 2.5–6V |
| Operating current | 130–220 mA |
| Flow rate | 80–120 L/H |
| Power | 0.4–1.5W |
| Hydraulic head | 40–110 cm |
| Dimensions | 56mm (L) × 24mm (D) × 33mm (H) |
| Inlet/outlet inner diameter | 4.5 mm |
| Weight | 30 g |

**Key notes:**
- Submersible — water level must sit *above* the pump
- Not self-priming — water must flow in before the pump runs
- Keep the water clean; clean the pump rotor regularly

**Next:** Figure out how to connect the pump to the Pi and integrate it into the full setup.

---

## Open Problems

### Problem 2 — Tubing

We have roughly 1m of 6mm tubing and don't want to cut it down too much. Both the pump and the flow sensor have inlets and outlets, so we need at least four tube connections total. Consider ordering more 6mm tubing.

### Problem 3 — Physical Layout

Need to decide:
- Positioning of the pump relative to the water source (pump must be submerged / below water level)
- Positioning of the flow sensor relative to the pump and nozzle
- Distance between components along the tube run
- Whether distance to the nozzle or distance from the water source affects accuracy or flow behaviour

Both components support 6mm tubing, so the main decisions are about tube lengths and component placement.

### Problem 4 — GPIO / Voltage Levels

The ultrasonic sensor needs a voltage divider to interface with the Pi's 3.3V GPIO. Need to check whether the same applies to the YF-S401 flow meter.

**YF-S401 Flow Sensor specs:**
| Parameter | Value |
|---|---|
| Working voltage | DC 5V–18V |
| Max current | 15 mA @ 5V |
| Load capability | 10 mA @ 5V |
| Flow range | 0.3–6 L/min |
| Water pressure | < 1.75 MPa |
| Connector | G1/4", fits 5–7mm ID tubing (6mm ideal) |

Included silicone tube: 30 cm, 6mm ID.

---

## Reference: Raspberry Pi 5 (Our Board)

| Feature | Spec |
|---|---|
| Processor | BCM2712 2.4GHz quad-core Cortex-A76 |
| Memory | 4GB LPDDR4X |
| GPIO | Standard 40-pin (3.3V logic) |
| USB | 2x USB 3.0, 2x USB 2.0 |
| Power | 5V/5A USB-C (PD required) |
| Extras | Onboard RTC, power button, fan header |
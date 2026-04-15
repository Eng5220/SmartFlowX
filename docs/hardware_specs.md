# Hardware Specifications

A reference document for the various hardware components used in the AquaFlow project.

---

## Pump: JT80SL DC Submersible Mini Water Pump
- **Electrical:**
  - Voltage: DC 2.5–6V
  - Current: 130–220 mA
  - Power: 0.4–1.5W
- **Performance:**
  - Flow rate: 80–120 L/H
  - Max head (hydraulic lift): 40–110 cm
- **Physical:**
  - Length: 56 mm (including water inlet)
  - Height: 33 mm (including water outlet)
  - Diameter: 24 mm
  - Weight: 30 g
  - Water inlet/outlet inner diameter: 4.5 mm (both)
  - Water inlet outer diameter: 6.8 mm
  - Water outlet outer diameter: 7 mm
- **Operating Notes:**
  - Submersible only - not self-priming; water level must be above the pump at all times.
  - Running dry causes overheating and noise.
  - Rotor can clog; clean regularly if water quality is low.

---

## Sensor: YF-S401 Hall Effect Water Flow Sensor
- **Electrical:**
  - Operating voltage: DC 5V–18V (datasheet min: 3.5V, practically works at 3.3V per field reports)
  - Max current draw: 15 mA (at DC 5V)
  - Signal output: Square wave pulse (Hall effect, open collector)
- **Performance:**
  - Flow range: 0.3–6 L/min
  - Pulse rate: ~5880 pulses/litre (~0.17 mL per pulse)
  - Max water pressure: 1.75 MPa
- **Physical:**
  - Dimensions: 58 × 35 × 27 mm
  - Weight: 25 g
  - Connection: G1/4" fitting, compatible with 5–7 mm ID hose (6 mm optimal)
  - Included: 30 cm silicone tubing (6 mm ID)
- **Wiring (3-wire):**
  - Red: VCC (5V)
  - Black: GND
  - Yellow: Signal (pulse output)
- **Operating Notes:**
  - Hall effect sensor is sealed from the water path - stays dry.
  - Not self-priming; designed for inline use in an active flow path.
  - Clean water only; avoid chemical exposure and physical shock.

---

## Gesture Sensor: DollaTek APDS-9960
- **Features:** Sensor for ambient light, RGB detection, proximity, gesture, and motion recognition.
- **Brand:** DollaTek
- **Power:** Corded Electric; operates at 3.3 V 
- **Weight:** 0.08 kg
- **Interaction:**
  - Detects ambient light and RGB colors accurately.
  - Recognizes gestures: Up, Down, Left, Right.
  - Detection range: 100–200mm. 
- **Hardware Integration:**
  - Communicates via I2C interface.
  - Features infrared LED, proximity sensor.
  - Pins: VCC/GND (3.3 V power), SDA/SCL (I2C), INT (interrupt), VL (infrared LED power).
  - Board size: 15.5 x 20.5 mm.

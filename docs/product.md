# Product Requirements & Plan

This document serves as the master plan for what we are building with the AquaFlow water dispenser project.

## Core Objective
Have the pump, water flow sensor, gesture sensor (APDS-9960), and LCD all working together in our water dispenser (Note: the ultrasonic sensor has been removed in favor of a simpler, gesture-sensor-only hardware layout). The system should use the pump to automatically draw water from a bottle into a cup.

## Target Audience / Setting
- Designed for a restaurant setting (e.g., McDonald's or KFC).
- Key feature: Touchless interaction (hygienic, COVID-friendly approach similar to non-touch doors).

## Interaction Models
We have two approaches for the user interaction via the gesture sensor (APDS-9960):
1. **Fixed Volume / Cup Sizes:** 
   Assign three different gestures for fixed cup sizes (Small, Medium, Large).
2. **Direct Pour Control:** 
   Use gestures to control the start and stop of water pouring directly.

## Smart Hardware Layout & Workflow
The gesture sensor (APDS-9960) acts as a physical **Cup Detector**: When the user slides the cup onto the platform, the cup will sit right in front of the sensor. The sensor's proximity reading will immediately spike, telling your Raspberry Pi, "A cup is parked here, we are safe to pour!"

**Protection from Water:**  
We mount it horizontally on the back wall so it faces the cup natively, protecting the module from water splashes and condensation.

**Intuitive Swiping:**  
The user waves their hand in the hollow bay area (Left, Up, Right) to select their size when there is no cup present, and then places the cup in to start the flow.

### How the Touchless Workflow Operates (No Ultrasonic Needed)
With the pump submerged in the reservoir and the APDS-9960 mounted on the back wall, the system operates as follows:

1. **Idle State:** The APDS is scanning for gestures.
2. **User Approaches:** The user swipes Right. The Pi logs that the user selected a Large (500ml) drink. This selection appears on the LCD screen. Placing the cup closer acts as physical confirmation that this is what the user wants.
3. **Cup Placement:** The user places their cup on the platform. The APDS proximity reading jumps above a certain threshold (e.g., > 150).
4. **Dispense:** The Pi triggers the JT80SL Pump via the `PumpController`. The pump pushes water up the tube.
5. **Monitor:** The Flow sensor spins and counts milliliters.
6. **Stop:** Once the target volume (e.g., 500ml) is reached, the Pi instantly cuts power to the pump. *Note: If the proximity reading drops before the target volume is reached (meaning the user pulled the cup out early), the Pi acts as an emergency stop and instantly shuts off the pump to save the floor from a puddle.*

This creates a brilliant, bulletproof touchless system using the absolute minimum amount of hardware.

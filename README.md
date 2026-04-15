# SmartFlowX💧

> **Real-time automated filling system with intelligent level detection and precision flow control.**

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%205-red.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)
![OS](https://img.shields.io/badge/OS-Linux%20%28Raspberry%20Pi%20OS%29-green.svg)
![Status](https://img.shields.io/badge/status-active-brightgreen.svg)

---

## 📌 Overview

**SmartFlow** is a real-time automated filling system built in **C++** and running natively on **Linux (Raspberry Pi OS)** on a **Raspberry Pi 5**. It leverages the Pi's GPIO interface to communicate with level sensors and control pumps/valves, delivering a fully automated, precise, and intelligent filling solution — with no manual intervention required.

---

## ✨ Features

- 🔄 **Real-Time Level Monitoring** — Continuous polling of liquid levels via GPIO-connected sensors
- 🧠 **Smart Level Detection** — Threshold-based logic triggers automatic fill and stop actions
- ⚙️ **Automated Flow Control** — Relay-controlled pump and valve management
- 📊 **Live Terminal Dashboard** — Real-time status output directly in the Linux terminal
- 🔔 **Alerts & Notifications** — System warnings for overflow, underflow, and sensor faults
- 📁 **Data Logging** — CSV-based event and fill history logging
- 🔌 **GPIO Native Integration** — Direct hardware control via `lgpio` / `gpiod` on Raspberry Pi 5
- 🧵 **Multithreaded Architecture** — Sensor reading and control logic run on separate threads

---

## 🛠️ Tech Stack

| Layer | Technology |
|-------|------------|
| Language | C++17 |
| Hardware | Raspberry Pi 5 |
| Operating System | Raspberry Pi OS (Linux) |
| GPIO Library | `lgpio` / `libgpiod` |
| Build System | CMake 3.16+ |
| Level Sensor | Ultrasonic (HC-SR04) / Float Switch |
| Actuator Control | 5V Relay Module (Pump & Valve) |
| Communication | I2C / GPIO Digital Pins |
| Data Logging | CSV file logging OR DB for IoT|


---

## 🔧 Hardware Requirements

| Component | Details |
|-----------|---------|
| **Raspberry Pi 5** | 4GB RAM |
| **Ultrasonic Sensor** | HC-SR04 (level detection) |
| **Float Switch** | Optional backup level detection |
| **Relay Module** | 5V single or dual channel |
| **Water Pump** | 5V/12V DC submersible pump |
| **Solenoid Valve** | 12V normally-closed valve |
| **Power Supply** | 5V 5A USB-C (Pi 5) + 12V for actuators |

---

## 📐 GPIO Pin Mapping

| GPIO Pin | Component | Direction |
|----------|-----------|-----------|
| GPIO 17 | HC-SR04 TRIG | Output |
| GPIO 27 | HC-SR04 ECHO | Input |
| GPIO 22 | Relay (Pump) | Output |
| GPIO 23 | Relay (Valve) | Output |
| GPIO 24 | Float Switch | Input |

> ⚠️ Always use a **voltage divider** on the ECHO pin (HC-SR04 outputs 5V; RPi GPIO is 3.3V max).

---

## 🚀 Getting Started

### 1. Prerequisites

Ensure your Raspberry Pi 5 is running **Raspberry Pi OS (Linux)** and has the following installed:

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y cmake g++ libgpiod-dev liblgpio-dev
```

### 2. Clone the Repository

```bash
git clone https://github.com/Eng5220/SmartFlowX.git
cd smartflow
```

### 3. Build the Project

```bash
mkdir build && cd build
cmake ..
make -j4
```

### 4. Run SmartFlow

```bash
sudo ./smartflow
```

> ⚠️ `sudo` is required for GPIO access on Raspberry Pi OS.

---

## 🔄 How It Works

1. **Sensor** reads liquid level every 500ms via the HC-SR04 ultrasonic sensor on GPIO
2. **Detection Logic** compares the reading against `threshold_high` and `threshold_low`
3. **Controller** dispatches commands to start or stop the pump/valve via relay
4. **Actuator** toggles GPIO pins to physically control the pump and solenoid valve
5. **Logger** records every fill event, level reading, and system alert to a CSV file
6. **Terminal Dashboard** prints live status, current fill %, and system state

---

## 🧪 Running Tests

```bash
cd build
make test
./smartflow_tests
```

---

## 🤝 Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a new branch (`git checkout -b feature/your-feature`)
3. Commit your changes (`git commit -m 'Add your feature'`)
4. Push to the branch (`git push origin feature/your-feature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).

---

## 📬 Contact

**SmartFlow Team**
- GitHub: [@Eng5220/SmartFlowX](https://github.com/Eng5220/SmartFlowX)
- TEAM:20
<img src="./images/SmartFlowXIcon.png" alt="SmartFlowX Prototype" align="right" width="100">
---

<p align="center">Built with precision on Raspberry Pi 5. Powered by C++ and Linux. 🐧💡</p>

# Hardware Wiring & Pinout Guide
### Project: Smart Parking Slot Detection System (ESP32 NodeMCU 30-Pin CP2102)
**Board Model:** DOIT ESP32 DEVKIT V1 / NodeMCU-32S (30 Pins, Silicon Labs CP2102 USB-to-UART)

---

## 1. Pin Mapping Summary Table

| Peripheral Component | Pin Label on Component | Connected to ESP32 Pin | GPIO # | Role / Mode | Safety Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Slot 1 IR Sensor** | OUT | **D25** | `GPIO 25` | Digital INPUT | Safe general I/O |
| | VCC | **3V3** (or VIN*) | — | Power (3.3V) | *Use 3.3V to keep signal level safe for ESP32 |
| | GND | **GND** | — | Common Ground | |
| **Slot 2 IR Sensor** | OUT | **D26** | `GPIO 26` | Digital INPUT | Safe general I/O |
| | VCC | **3V3** (or VIN*) | — | Power (3.3V) | *Use 3.3V to keep signal level safe for ESP32 |
| | GND | **GND** | — | Common Ground | |
| **Slot 1 Green LED** | Anode (+) via 220Ω | **D18** | `GPIO 18` | Digital OUTPUT | Indicates Slot 1 is **FREE** |
| | Cathode (-) | **GND** | — | Ground Rail | |
| **Slot 1 Red LED** | Anode (+) via 220Ω | **D19** | `GPIO 19` | Digital OUTPUT | Indicates Slot 1 is **OCCUPIED** |
| | Cathode (-) | **GND** | — | Ground Rail | |
| **Slot 2 Green LED** | Anode (+) via 220Ω | **D21** | `GPIO 21` | Digital OUTPUT | Indicates Slot 2 is **FREE** |
| | Cathode (-) | **GND** | — | Ground Rail | |
| **Slot 2 Red LED** | Anode (+) via 220Ω | **D22** | `GPIO 22` | Digital OUTPUT | Indicates Slot 2 is **OCCUPIED** |
| | Cathode (-) | **GND** | — | Ground Rail | |

---

## 2. Why These Specific Pins Were Chosen (Strapping & Boot Pin Avoidance)

The 30-pin ESP32 NodeMCU has several critical pin restrictions that must be respected to prevent boot failures or hardware crashes:

1. **Strapping Pins Avoided:**
   - `GPIO 0`: Bootloader mode switch. If pulled low at startup, ESP32 enters download mode and firmware will not run.
   - `GPIO 2`: Connected to onboard blue LED and strapped.
   - `GPIO 12 (MTDI)`: **Critical**. If pulled HIGH at boot, sets internal flash voltage to 1.8V instead of 3.3V, causing a boot loop.
   - `GPIO 15`: Outputs boot debug log.
2. **Input-Only Pins Avoided for LEDs:**
   - `GPIO 34, 35, 36 (VP), 39 (VN)`: These lack internal pullup/pulldown resistors and output drivers. They **cannot** drive LEDs.
3. **UART Pins Preserved:**
   - `GPIO 1 (TX0)` & `GPIO 3 (RX0)`: Left dedicated to the CP2102 chip for high-speed 115200 baud Serial debugging.

**Selected Pins (`25, 26, 18, 19, 21, 22`):**
- All 6 pins have full bi-directional push-pull output drivers and software pullup capability.
- Completely neutral during the ESP32 boot sequence.

---

## 3. Step-by-Step Breadboard Wiring

### A. Power Rails
1. Connect ESP32 **3V3** pin to the **(+) red rail** on your breadboard.
2. Connect ESP32 **GND** pin to the **(-) blue/black rail** on your breadboard.

### B. IR Obstacle Sensors (LM393 3-Pin)
> **Calibration Tip:** The LM393 module has a small blue potentiometer. Turn it using a small screwdriver until the onboard green indicator LED turns OFF when no object is nearby, and turns ON when a hand/toy car is 3–8 cm in front of the IR emitter/receiver pair.

- **Sensor 1 (Slot 1):**
  - `VCC` ➔ Breadboard `3V3` (+) rail
  - `GND` ➔ Breadboard `GND` (-) rail
  - `OUT` ➔ ESP32 pin **D25**
- **Sensor 2 (Slot 2):**
  - `VCC` ➔ Breadboard `3V3` (+) rail
  - `GND` ➔ Breadboard `GND` (-) rail
  - `OUT` ➔ ESP32 pin **D26**

### C. Status LEDs (with 220Ω or 330Ω Resistors)
> Remember: The **longer leg** of an LED is the Anode (+), and the **shorter leg** (or flat side) is the Cathode (-).

1. **Slot 1 Green LED (Free):**
   - Anode (+) ➔ 220Ω resistor ➔ ESP32 pin **D18**
   - Cathode (-) ➔ Breadboard `GND` rail
2. **Slot 1 Red LED (Occupied):**
   - Anode (+) ➔ 220Ω resistor ➔ ESP32 pin **D19**
   - Cathode (-) ➔ Breadboard `GND` rail
3. **Slot 2 Green LED (Free):**
   - Anode (+) ➔ 220Ω resistor ➔ ESP32 pin **D21**
   - Cathode (-) ➔ Breadboard `GND` rail
4. **Slot 2 Red LED (Occupied):**
   - Anode (+) ➔ 220Ω resistor ➔ ESP32 pin **D22**
   - Cathode (-) ➔ Breadboard `GND` rail

---

## 4. CP2102 Driver on Windows
Because your board uses the **CP2102 USB-to-UART bridge**:
- When plugged into your Windows PC via Micro-USB, it should appear in Device Manager under `Ports (COM & LPT)` as `Silicon Labs CP210x USB to UART Bridge (COMx)`.
- If a yellow exclamation mark appears, install the driver from the official Silicon Labs website:
  `https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers`

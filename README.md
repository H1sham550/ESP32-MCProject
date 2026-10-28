# Smart Parking Slot Detection System (IoT Embedded System)
**Semester 5 Computer Science & Engineering (S5 CSE) Microcontroller Project**

**Team Members:** Alan Biju, Ashwin Roy, Dibin Dinesh, Hisham T S, Siva Priya Rajeesh

---

## 📌 Project Overview
An intelligent, real-time parking slot detection system featuring:
- **Edge Controller:** ESP32 NodeMCU (30-pin, CP2102) reading 2× LM393 IR Obstacle Sensors with non-blocking software debouncing.
- **Physical Signaling:** 4× Status LEDs (Red for Occupied, Green for Free) driven via safe push-pull GPIOs.
- **IoT Network Layer:** Connects to mobile phone hotspot via Static IP (`192.168.43.50`), running a low-latency HTTP REST API (`GET /slots`).
- **Telemetry:** Real-time USART log streaming at 115200 baud.
- **Android Mobile Application:** Live visual cards, available slot counter, dynamic IP settings, and cleartext HTTP network configuration.

---

## 📂 Project Directory Structure

```text
Microcontrollers/
├── esp32_firmware/
│   ├── esp32_firmware.ino         # Complete Arduino C++ firmware (Wi-Fi, HTTP Server, Debounce, LEDs)
│   └── platformio.ini             # PlatformIO config for VS Code embedded development
├── mobile_app/
│   └── flutter_parking_app/       # Android Companion App (Flutter)
│       ├── lib/main.dart          # Live parking UI, 800ms polling, IP configuration modal
│       ├── pubspec.yaml           # Dependencies (http, cupertino_icons)
│       └── android/app/src/main/
│           └── AndroidManifest.xml # Configured with usesCleartextTraffic="true"
└── docs/
    ├── WIRING_AND_PINOUT.md       # Pin mapping, strapping pin analysis, breadboard guide
    └── ACADEMIC_MAPPING_STM32_TO_ESP32.md # Defense & mapping of STM32 proposal to ESP32
```

---

## ⚡ Hardware Pinout (Safe 30-pin NodeMCU Mapping)

| Function | Pin on NodeMCU (30-Pin) | GPIO Number | Safe Status |
| :--- | :--- | :--- | :--- |
| **IR Sensor 1 (Slot 1)** | `D25` | `GPIO 25` | ✅ Safe Input |
| **IR Sensor 2 (Slot 2)** | `D26` | `GPIO 26` | ✅ Safe Input |
| **Green LED (Slot 1 Free)** | `D18` | `GPIO 18` | ✅ Safe Output |
| **Red LED (Slot 1 Occupied)** | `D19` | `GPIO 19` | ✅ Safe Output |
| **Green LED (Slot 2 Free)** | `D21` | `GPIO 21` | ✅ Safe Output |
| **Red LED (Slot 2 Occupied)** | `D22` | `GPIO 22` | ✅ Safe Output |
| **PC Serial Monitoring** | `TX0` / `RX0` (via USB CP2102) | `GPIO 1 / 3` | ✅ 115200 Baud |

*Strapping pins (GPIO 0, 2, 12, 15) and input-only pins (GPIO 34–39) are completely isolated from outputs.*

---

## 🚀 Quick Setup & Demonstration Guide

### 1. Flash the ESP32
1. Open `esp32_firmware/esp32_firmware.ino` in **Arduino IDE** (or open the folder in **VS Code** with PlatformIO).
2. Under Arduino IDE **Tools > Board**, choose `DOIT ESP32 DEVKIT V1` (or `ESP32 Dev Module`).
3. Set your phone hotspot credentials in lines 36–37:
   ```cpp
   const char* WIFI_SSID     = "Your_Phone_Hotspot_Name";
   const char* WIFI_PASSWORD = "Your_Hotspot_Password";
   ```
4. Ensure your phone hotspot is set to **2.4 GHz Band** (ESP32 does not support 5 GHz Wi-Fi).
5. Plug the ESP32 into your PC via Micro-USB, select the CP2102 COM port, and click **Upload**.
6. Open Serial Monitor at **115200 baud** to verify static IP assignment (`192.168.43.50`).

### 2. Verify REST API
Once connected to your phone's hotspot:
- Open Chrome or any browser on your phone and visit:
  `http://192.168.43.50/slots`
- You will receive real-time JSON:
  ```json
  {"slot1":"FREE","slot2":"OCCUPIED","available":1,"uptime_ms":14250}
  ```
- You can also visit `http://192.168.43.50/` to test the built-in mobile dashboard immediately!

### 3. Run the Android Mobile App
1. Open `mobile_app/flutter_parking_app` in VS Code or Android Studio.
2. Run `flutter pub get`.
3. Connect your Android phone via USB (with Developer Options > USB Debugging enabled) and run:
   ```bash
   flutter run
   ```
   Or generate the APK directly:
   ```bash
   flutter build apk --split-per-abi
   ```
   The `.apk` file will be generated in `build/app/outputs/flutter-apk/app-armeabi-v7a-release.apk` (or `app-arm64-v8a-release.apk`).
4. Transfer and install the `.apk` on your phone!

/**
 * ==============================================================================
 * Project: Smart Parking Slot Detection System (S5 CSE Embedded IoT)
 * Target Hardware: ESP32 NodeMCU (30-pin Development Board)
 * Author / Team: Alan Biju, Ashwin Roy, Dibin Dinesh, Hisham T S, Siva Priya Rajeesh
 * 
 * Pin Configuration (Safe GPIOs - Strapping & Input-only pins avoided):
 *   - Slot 1 IR Sensor : GPIO 25 (INPUT)
 *   - Slot 2 IR Sensor : GPIO 26 (INPUT)
 *   - Slot 1 Green LED : GPIO 18 (OUTPUT) - Slot 1 FREE
 *   - Slot 1 Red LED   : GPIO 19 (OUTPUT) - Slot 1 OCCUPIED
 *   - Slot 2 Green LED : GPIO 21 (OUTPUT) - Slot 2 FREE
 *   - Slot 2 Red LED   : GPIO 22 (OUTPUT) - Slot 2 OCCUPIED
 * 
 * Sensor Logic:
 *   - LM393 IR Obstacle Sensor OUT pin is Active LOW:
 *       LOW  (0) = Obstacle / Vehicle Detected -> OCCUPIED
 *       HIGH (1) = Beam Clear / No Vehicle     -> FREE
 * 
 * Networking:
 *   - Connects to Mobile Hotspot with Static IP: 192.168.43.50
 *   - Lightweight HTTP Server on Port 80:
 *       GET /slots  -> JSON payload for Companion Mobile App
 *       GET /status -> JSON payload
 *       GET /       -> Built-in Live Web Dashboard for instant browser testing
 * ==============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ======================== PIN DEFINITIONS ========================
const int PIN_IR_SLOT1       = 25;  // Safe ADC2/GPIO (Input)
const int PIN_IR_SLOT2       = 26;  // Safe ADC2/GPIO (Input)

const int PIN_LED_SLOT1_GRN  = 18;  // Slot 1 FREE Indicator
const int PIN_LED_SLOT1_RED  = 19;  // Slot 1 OCCUPIED Indicator
const int PIN_LED_SLOT2_GRN  = 21;  // Slot 2 FREE Indicator
const int PIN_LED_SLOT2_RED  = 22;  // Slot 2 OCCUPIED Indicator

// ===================== WI-FI & STATIC IP CONFIG =====================
// Replace with your Phone Hotspot credentials:
const char* WIFI_SSID     = "YOUR_HOTSPOT_SSID";
const char* WIFI_PASSWORD = "YOUR_HOTSPOT_PASSWORD";

// Static IP Configuration (Default Android Hotspot Subnet: 192.168.43.x)
IPAddress local_IP(192, 168, 43, 50);
IPAddress gateway(192, 168, 43, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 43, 1);
IPAddress secondaryDNS(8, 8, 8, 8);

// ======================= WEB SERVER INSTANCE =======================
WebServer server(80);

// ======================= STATE & DEBOUNCE ==========================
// Debounce duration in milliseconds
const unsigned long DEBOUNCE_DELAY_MS = 50;

struct SlotSensor {
  int pin;
  int rawState;
  int debouncedState;
  unsigned long lastDebounceTime;
  bool isOccupied;
};

SlotSensor slot1 = { PIN_IR_SLOT1, HIGH, HIGH, 0, false };
SlotSensor slot2 = { PIN_IR_SLOT2, HIGH, HIGH, 0, false };

// ====================== FUNCTION DECLARATIONS ======================
void updateSensor(SlotSensor &slot, const char* slotName);
void updateLEDs();
void handleGetSlots();
void handleGetStatus();
void handleRootDashboard();
void handleNotFound();
void setCORSHeaders();

// ============================== SETUP ==============================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("==================================================");
  Serial.println("  Smart Parking Slot Detection System - ESP32     ");
  Serial.println("==================================================");

  // 1. Initialize GPIO Pins
  pinMode(PIN_IR_SLOT1, INPUT);
  pinMode(PIN_IR_SLOT2, INPUT);

  pinMode(PIN_LED_SLOT1_GRN, OUTPUT);
  pinMode(PIN_LED_SLOT1_RED, OUTPUT);
  pinMode(PIN_LED_SLOT2_GRN, OUTPUT);
  pinMode(PIN_LED_SLOT2_RED, OUTPUT);

  // Initial LED Test (Blink all LEDs once)
  digitalWrite(PIN_LED_SLOT1_GRN, HIGH);
  digitalWrite(PIN_LED_SLOT1_RED, HIGH);
  digitalWrite(PIN_LED_SLOT2_GRN, HIGH);
  digitalWrite(PIN_LED_SLOT2_RED, HIGH);
  delay(500);
  digitalWrite(PIN_LED_SLOT1_GRN, LOW);
  digitalWrite(PIN_LED_SLOT1_RED, LOW);
  digitalWrite(PIN_LED_SLOT2_GRN, LOW);
  digitalWrite(PIN_LED_SLOT2_RED, LOW);

  // 2. Configure Static IP & Connect to Wi-Fi
  Serial.print("[WiFi] Configuring Static IP: ");
  Serial.println(local_IP);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("[WiFi] Static IP configuration failed! Falling back to DHCP.");
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Connecting to: ");
  Serial.println(WIFI_SSID);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 25) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Connected successfully!");
    Serial.print("[WiFi] Assigned IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("[WiFi] RSSI Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("[WiFi] Connection timed out! Check SSID/Password or Hotspot AP band (must be 2.4 GHz).");
    Serial.println("[WiFi] System will continue running local sensor & LED logic.");
  }

  // 3. Register HTTP REST API Endpoints
  server.on("/slots", HTTP_GET, handleGetSlots);
  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/", HTTP_GET, handleRootDashboard);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("[HTTP] REST Server started on port 80.");
  Serial.println("[HTTP] Endpoints available:");
  Serial.println("       GET http://192.168.43.50/slots");
  Serial.println("       GET http://192.168.43.50/status");
  Serial.println("       GET http://192.168.43.50/ (Mobile Web UI)");
  Serial.println("==================================================");
}

// =============================== LOOP ==============================
void loop() {
  // Handle incoming HTTP client requests
  server.handleClient();

  // Read and debounce IR sensors
  updateSensor(slot1, "Slot 1");
  updateSensor(slot2, "Slot 2");

  // Update physical LED indicators
  updateLEDs();

  // Small yield to allow background Wi-Fi stack tasks to process
  delay(10);
}

// ======================= SENSOR & DEBOUNCE =========================
void updateSensor(SlotSensor &slot, const char* slotName) {
  int reading = digitalRead(slot.pin);

  if (reading != slot.rawState) {
    slot.lastDebounceTime = millis();
    slot.rawState = reading;
  }

  if ((millis() - slot.lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != slot.debouncedState) {
      slot.debouncedState = reading;
      // Active LOW: LOW = Vehicle Present (OCCUPIED), HIGH = Clear (FREE)
      bool previouslyOccupied = slot.isOccupied;
      slot.isOccupied = (slot.debouncedState == LOW);

      if (previouslyOccupied != slot.isOccupied) {
        Serial.printf("[SENSOR] %s State Changed -> %s (Raw Pin: %d)\n", 
                      slotName, 
                      slot.isOccupied ? "OCCUPIED" : "FREE", 
                      reading);
      }
    }
  }
}

// ======================== LED CONTROLLER ===========================
void updateLEDs() {
  // Slot 1 LEDs
  if (slot1.isOccupied) {
    digitalWrite(PIN_LED_SLOT1_RED, HIGH);
    digitalWrite(PIN_LED_SLOT1_GRN, LOW);
  } else {
    digitalWrite(PIN_LED_SLOT1_RED, LOW);
    digitalWrite(PIN_LED_SLOT1_GRN, HIGH);
  }

  // Slot 2 LEDs
  if (slot2.isOccupied) {
    digitalWrite(PIN_LED_SLOT2_RED, HIGH);
    digitalWrite(PIN_LED_SLOT2_GRN, LOW);
  } else {
    digitalWrite(PIN_LED_SLOT2_RED, LOW);
    digitalWrite(PIN_LED_SLOT2_GRN, HIGH);
  }
}

// ====================== HTTP REST HANDLERS =========================
void setCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

String buildJsonPayload() {
  const char* s1 = slot1.isOccupied ? "OCCUPIED" : "FREE";
  const char* s2 = slot2.isOccupied ? "OCCUPIED" : "FREE";
  int availableCount = (!slot1.isOccupied ? 1 : 0) + (!slot2.isOccupied ? 1 : 0);

  String json = "{";
  json += "\"slot1\":\"" + String(s1) + "\",";
  json += "\"slot2\":\"" + String(s2) + "\",";
  json += "\"available\":" + String(availableCount) + ",";
  json += "\"uptime_ms\":" + String(millis());
  json += "}";
  return json;
}

void handleGetSlots() {
  setCORSHeaders();
  String json = buildJsonPayload();
  server.send(200, "application/json", json);
  Serial.printf("[HTTP] 200 OK GET /slots -> %s\n", json.c_str());
}

void handleGetStatus() {
  handleGetSlots(); // Alias endpoint
}

void handleNotFound() {
  setCORSHeaders();
  server.send(404, "application/json", "{\"error\":\"Endpoint not found\"}");
}

// Built-in Mobile Responsive Dashboard (Fallback for any browser)
void handleRootDashboard() {
  setCORSHeaders();
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Parking Monitor</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: #0f172a; color: #f8fafc; padding: 20px; text-align: center; }
    .container { max-width: 480px; margin: 0 auto; }
    header { margin-bottom: 24px; padding-top: 10px; }
    h1 { font-size: 1.5rem; font-weight: 700; color: #38bdf8; }
    .subtitle { font-size: 0.85rem; color: #94a3b8; margin-top: 4px; }
    .counter-card { background: #1e293b; border-radius: 16px; padding: 20px; margin-bottom: 20px; border: 1px solid #334155; box-shadow: 0 4px 12px rgba(0,0,0,0.3); }
    .counter-val { font-size: 3rem; font-weight: 800; color: #22c55e; transition: all 0.3s; }
    .counter-label { font-size: 0.9rem; color: #94a3b8; text-transform: uppercase; letter-spacing: 1px; }
    .slots-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; margin-bottom: 24px; }
    .slot-card { background: #1e293b; border-radius: 16px; padding: 24px 16px; border: 2px solid #334155; transition: all 0.3s ease; }
    .slot-card.free { border-color: #22c55e; background: rgba(34, 197, 94, 0.08); }
    .slot-card.occupied { border-color: #ef4444; background: rgba(239, 68, 68, 0.08); }
    .slot-id { font-size: 1.1rem; font-weight: 600; margin-bottom: 8px; }
    .badge { display: inline-block; padding: 6px 14px; border-radius: 9999px; font-size: 0.8rem; font-weight: 700; }
    .badge.free { background: #22c55e; color: #052e16; }
    .badge.occupied { background: #ef4444; color: #450a0a; }
    .status-bar { font-size: 0.75rem; color: #64748b; margin-top: 12px; }
    .dot { display: inline-block; width: 8px; height: 8px; border-radius: 50%; background: #22c55e; margin-right: 6px; animation: pulse 1.5s infinite; }
    @keyframes pulse { 0% { opacity: 0.4; } 50% { opacity: 1; } 100% { opacity: 0.4; } }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>🅿️ Smart Parking System</h1>
      <p class="subtitle">ESP32 Live IoT Node | 192.168.43.50</p>
    </header>

    <div class="counter-card">
      <div class="counter-val" id="avail-count">-</div>
      <div class="counter-label">Slots Available</div>
    </div>

    <div class="slots-grid">
      <div class="slot-card" id="card-s1">
        <div class="slot-id">Slot 1</div>
        <div class="badge" id="badge-s1">CONNECTING</div>
      </div>
      <div class="slot-card" id="card-s2">
        <div class="slot-id">Slot 2</div>
        <div class="badge" id="badge-s2">CONNECTING</div>
      </div>
    </div>

    <div class="status-bar">
      <span class="dot"></span> Live Polling active (Every 800ms)
    </div>
  </div>

  <script>
    async function fetchSlots() {
      try {
        const res = await fetch('/slots');
        const data = await res.json();
        
        document.getElementById('avail-count').innerText = data.available;
        
        const updateSlot = (slotKey, cardId, badgeId) => {
          const card = document.getElementById(cardId);
          const badge = document.getElementById(badgeId);
          const isFree = data[slotKey] === "FREE";
          
          card.className = "slot-card " + (isFree ? "free" : "occupied");
          badge.className = "badge " + (isFree ? "free" : "occupied");
          badge.innerText = data[slotKey];
        };

        updateSlot('slot1', 'card-s1', 'badge-s1');
        updateSlot('slot2', 'card-s2', 'badge-s2');
      } catch (err) {
        console.error("Poll error", err);
      }
    }
    setInterval(fetchSlots, 800);
    fetchSlots();
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

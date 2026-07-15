/*
  VoltCharge Smart EV System — Node 1 (Charging Station)
  
  Board: ESP32 Dev Module
  Pin Configurations:
    - I2C LCD SDA   -> GPIO 21
    - I2C LCD SCL   -> GPIO 22
    - IR OUT Pin    -> GPIO 18 (Active LOW)
    - Relay IN Pin  -> GPIO 19 (Active HIGH)
    
  Dependencies:
    - LiquidCrystal_I2C Library
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Wi-Fi Credentials
const char* ssid = "Air";
const char* password = "99999999";

// Pin Declarations
const int IR_PIN = 18;
const int RELAY_PIN = 19;

// LCD Setup: 16 columns, 2 rows. Address 0x27 is standard.
LiquidCrystal_I2C lcd(0x27, 16, 2);

// State Variables
bool carPresent = false;
bool relayState = false;
String systemState = "idle"; // "idle", "ready", "charging"

// Display Parameters
String voltageVal = "0.0";
String currentVal = "0.0";
String batteryVal = "0";
String kwhVal = "0.0000";

WebServer server(80);

// Add CORS headers to response
void sendCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleOptions() {
  sendCORSHeaders();
  server.send(200, "text/plain", "");
}

// Update LCD screen based on current state and parameters
void updateLCD() {
  lcd.clear();
  if (systemState == "idle") {
    lcd.setCursor(0, 0);
    lcd.print("  VoltCharge EV ");
    lcd.setCursor(0, 1);
    lcd.print("Charging Station");
  } 
  else if (systemState == "ready") {
    lcd.setCursor(0, 0);
    lcd.print(" Welcome to EV  ");
    lcd.setCursor(0, 1);
    lcd.print("Charging Station");
  } 
  else if (systemState == "charging") {
    // Top Row: 12V 2.0A   45%
    lcd.setCursor(0, 0);
    lcd.print(voltageVal + "V " + currentVal + "A  " + batteryVal + "%");
    // Bottom Row: 0.0124 kWh
    lcd.setCursor(0, 1);
    lcd.print(kwhVal + " kWh");
  }
}

// Endpoints
void handleStatus() {
  sendCORSHeaders();
  int irVal = digitalRead(IR_PIN);
  bool isCarHere = (irVal == LOW); // IR standard output is active-low
  
  String json = "{\"car_present\":" + String(isCarHere ? "true" : "false") + ",";
  json += "\"relay_state\":" + String(relayState ? "true" : "false") + ",";
  json += "\"system_state\":\"" + systemState + "\"}";
  server.send(200, "application/json", json);
}

void handleRelayOn() {
  sendCORSHeaders();
  relayState = true;
  digitalWrite(RELAY_PIN, HIGH); // Turn relay ON
  systemState = "charging";
  updateLCD();
  server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Relay ON, charging started\"}");
}

void handleRelayOff() {
  sendCORSHeaders();
  relayState = false;
  digitalWrite(RELAY_PIN, LOW); // Turn relay OFF
  
  // Re-evaluate state based on IR sensor
  int irVal = digitalRead(IR_PIN);
  if (irVal == LOW) {
    systemState = "ready";
  } else {
    systemState = "idle";
  }
  
  updateLCD();
  server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Relay OFF, charging stopped\"}");
}

void handleUpdate() {
  sendCORSHeaders();
  if (server.hasArg("v")) voltageVal = server.arg("v");
  if (server.hasArg("c")) currentVal = server.arg("c");
  if (server.hasArg("bat")) batteryVal = server.arg("bat");
  if (server.hasArg("kwh")) kwhVal = server.arg("kwh");
  
  systemState = "charging";
  updateLCD();
  server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"LCD display parameters updated\"}");
}

void setup() {
  Serial.begin(115200);
  
  // Pin Setup
  pinMode(IR_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Relay initially OFF
  
  // LCD Initialize
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Wait...");

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Web Server Routes Configuration
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/status", HTTP_OPTIONS, handleOptions);
  
  server.on("/relay/on", HTTP_GET, handleRelayOn);
  server.on("/relay/on", HTTP_OPTIONS, handleOptions);
  
  server.on("/relay/off", HTTP_GET, handleRelayOff);
  server.on("/relay/off", HTTP_OPTIONS, handleOptions);
  
  server.on("/update", HTTP_GET, handleUpdate);
  server.on("/update", HTTP_OPTIONS, handleOptions);
  
  server.begin();
  Serial.println("Node 1 (Station) Server Started.");
  
  // Initial LCD update
  int irVal = digitalRead(IR_PIN);
  if (irVal == LOW) {
    systemState = "ready";
  } else {
    systemState = "idle";
  }
  updateLCD();
}

void loop() {
  server.handleClient();
  
  // Periodically check IR sensor if not actively charging
  if (systemState != "charging") {
    int irVal = digitalRead(IR_PIN);
    bool isCarHere = (irVal == LOW);
    
    if (isCarHere != carPresent) {
      carPresent = isCarHere;
      if (carPresent) {
        systemState = "ready";
      } else {
        systemState = "idle";
      }
      updateLCD();
      Serial.print("IR Car Detection state changed: ");
      Serial.println(carPresent ? "CAR PRESENT" : "NO CAR");
    }
  }
  
  delay(100);
}

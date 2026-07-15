/*
  VoltCharge Smart EV System — Node 3 (Electric Car)
  
  Board: ESP32 Dev Module
  Pin Configurations:
    - I2C LCD SDA   -> GPIO 21
    - I2C LCD SCL   -> GPIO 22
    - Buzzer Pin    -> GPIO 23 (Active HIGH)
    
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
const int BUZZER_PIN = 23;

// LCD Setup: 16 columns, 2 rows. Address 0x27 is standard.
LiquidCrystal_I2C lcd(0x27, 16, 2);

// State Variables
int alignmentPercent = 0;
bool longBeepTriggered = false;
bool longBeepActive = false;
unsigned long longBeepStartTime = 0;

// Intermittent Beep Variables (for 1% to 99% alignment)
unsigned long lastBeepToggleTime = 0;
bool buzzerState = false;
const int beepOnTime = 150;   // milliseconds buzzer is active during beep
const int beepOffTime = 350;  // milliseconds buzzer is silent during beep

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

// Update LCD screen contents based on alignment percentage
void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Coil Alignment");
  
  lcd.setCursor(0, 1);
  if (alignmentPercent == 0) {
    lcd.print("0% - Aligning...");
  } 
  else if (alignmentPercent == 100) {
    lcd.print("Aligned: 100%");
  } 
  else {
    lcd.print("Aligning: ");
    lcd.print(alignmentPercent);
    lcd.print("%");
  }
}

// Endpoints
void handleStatus() {
  sendCORSHeaders();
  String json = "{\"status\":\"online\",\"alignment_pct\":" + String(alignmentPercent) + "}";
  server.send(200, "application/json", json);
}

void handleUpdate() {
  sendCORSHeaders();
  if (server.hasArg("percent")) {
    int newPercent = server.arg("percent").toInt();
    if (newPercent != alignmentPercent) {
      alignmentPercent = newPercent;
      updateLCD();
      Serial.print("Alignment updated: ");
      Serial.print(alignmentPercent);
      Serial.println("%");
    }
  }
  server.send(200, "application/json", "{\"status\":\"success\",\"alignment_pct\":" + String(alignmentPercent) + "}");
}

void setup() {
  Serial.begin(115200);
  
  // Pin Setup
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Start with buzzer OFF
  
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
  
  server.on("/update", HTTP_GET, handleUpdate);
  server.on("/update", HTTP_OPTIONS, handleOptions);
  
  server.begin();
  Serial.println("Node 3 (Car) Server Started.");
  
  // Initial LCD update
  updateLCD();
}

void loop() {
  server.handleClient();
  
  unsigned long currentMillis = millis();
  
  // Buzzer Control Logic
  if (alignmentPercent == 0) {
    // 0% alignment -> Buzzer completely silent
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
    longBeepTriggered = false;
    longBeepActive = false;
  } 
  else if (alignmentPercent > 0 && alignmentPercent < 100) {
    // 1% to 99% alignment -> Intermittent Beeping
    longBeepTriggered = false;
    longBeepActive = false;
    
    int currentPeriod = buzzerState ? beepOnTime : beepOffTime;
    if (currentMillis - lastBeepToggleTime >= currentPeriod) {
      lastBeepToggleTime = currentMillis;
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    }
  } 
  else if (alignmentPercent == 100) {
    // 100% alignment -> 2-second continuous long beep, then silence
    if (!longBeepTriggered) {
      digitalWrite(BUZZER_PIN, HIGH);
      longBeepStartTime = currentMillis;
      longBeepTriggered = true;
      longBeepActive = true;
      Serial.println("Coil 100% Aligned! Triggering 2-second beep...");
    } 
    else if (longBeepActive) {
      if (currentMillis - longBeepStartTime >= 2000) {
        digitalWrite(BUZZER_PIN, LOW);
        longBeepActive = false;
        Serial.println("2-second beep finished. Buzzer muted.");
      }
    } 
    else {
      // Long beep has completed, ensure buzzer remains off
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
  
  delay(10); // Quick loop resolution
}

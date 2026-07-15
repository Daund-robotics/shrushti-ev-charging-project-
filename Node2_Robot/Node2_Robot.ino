/*
  VoltCharge Smart EV System — Node 2 (Charging Robot Proximity Node)
  
  Board: ESP32 Dev Module
  Pin Configurations:
    - Ultrasonic TRIG  -> GPIO 4
    - Ultrasonic ECHO  -> GPIO 5
*/

#include <WiFi.h>
#include <WebServer.h>

// Wi-Fi Credentials
const char* ssid = "Air";
const char* password = "99999999";

// Pin Declarations
const int TRIG_PIN = 4;
const int ECHO_PIN = 5;

WebServer server(80);

// Helper function to read ultrasonic distance (cm)
float readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return 999.0; // No object detected
  
  float distance = duration * 0.034 / 2.0;
  return distance;
}

// Map distance to alignment percentage
// >= 10.0cm -> 0%
// <= 3.0cm  -> 100%
// Between 3.0 and 10.0cm: linear interpolation
int calculateAlignmentPercentage(float distance) {
  if (distance >= 10.0 || distance > 500.0) {
    return 0;
  }
  if (distance <= 3.0) {
    return 100;
  }
  float pct = ((10.0 - distance) / (10.0 - 3.0)) * 100.0;
  return (int)round(pct);
}

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

// Endpoints
void handleStatus() {
  sendCORSHeaders();
  float dist = readDistance();
  int alignPct = calculateAlignmentPercentage(dist);
  
  String json = "{\"state\":\"idle\",";
  json += "\"distance\":" + String(dist, 2) + ",";
  json += "\"alignment_pct\":" + String(alignPct) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  
  // Pin Modes
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
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
  
  server.begin();
  Serial.println("Node 2 (Proximity Sensor) Server Started.");
}

void loop() {
  server.handleClient();
  delay(50);
}

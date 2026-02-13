/*
 * Smart Study Environment System - Node 2
 * Lighting & Presence Monitoring + Environment Alert
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "arduino_secrets.h"  // Load WiFi credentials from the secret tab

// ===== WiFi Configuration =====
const char* WIFI_SSID = SECRET_SSID;
const char* WIFI_PASSWORD = SECRET_PASS;

// ===== Pin Definitions =====
#define PIN_LDR A0            
#define PIN_TRIG D1           
#define PIN_ECHO D2           
#define PIN_BUZZER D5          
#define PIN_LED_ALERT D6      
#define PIN_LED_PRESENCE D7   

// ===== HTTP Server =====
ESP8266WebServer server(80);

// ===== Monitoring Variables =====
int lightLevel = 0;
long distance = 0;
bool presenceDetected = false;
String alertStatus = "normal";

// ===== Thresholds =====
const int LIGHT_THRESHOLD = 300;     
const int DISTANCE_THRESHOLD = 80;   

// ===== Timing Control =====
unsigned long lastMeasureTime = 0;
const unsigned long MEASURE_INTERVAL = 1000; 

void setup() {
  Serial.begin(9600);
  delay(1000); // Wait for Serial to stabilize
  
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_ALERT, OUTPUT);
  pinMode(PIN_LED_PRESENCE, OUTPUT);
  
  digitalWrite(PIN_LED_ALERT, LOW);
  digitalWrite(PIN_LED_PRESENCE, LOW);
  
  connectToWiFi();
  
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  
  Serial.println("\nNode 2: Environment Monitor Server Started");
}

void loop() {
  server.handleClient();
  
  if (millis() - lastMeasureTime >= MEASURE_INTERVAL) {
    measureEnvironment();
    lastMeasureTime = millis();
  }
}

void measureEnvironment() {
  lightLevel = analogRead(PIN_LDR);
  
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  
  long duration = pulseIn(PIN_ECHO, HIGH, 26000);
  distance = duration * 0.034 / 2;
  
  presenceDetected = (distance > 0 && distance < DISTANCE_THRESHOLD);
  
  if (presenceDetected && lightLevel < LIGHT_THRESHOLD) {
    alertStatus = "too_dark";
    digitalWrite(PIN_LED_ALERT, HIGH);
    tone(PIN_BUZZER, 1000, 100); 
  } else {
    alertStatus = "normal";
    digitalWrite(PIN_LED_ALERT, LOW);
    noTone(PIN_BUZZER);
  }
  
  digitalWrite(PIN_LED_PRESENCE, presenceDetected ? HIGH : LOW);
  
  // Display data in Serial Monitor
  Serial.print("Light: "); Serial.print(lightLevel);
  Serial.print(" | Dist: "); Serial.print(distance);
  Serial.print("cm | Presence: "); Serial.println(presenceDetected ? "YES" : "NO");
}

void handleRoot() {
  String html = "<h1>Node 2: Environment Monitor</h1>";
  html += "<p>Light Level: " + String(lightLevel) + "</p>";
  html += "<p>Presence: " + String(presenceDetected ? "Detected" : "None") + "</p>";
  html += "<p>Status: " + alertStatus + "</p>";
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"light_level\":" + String(lightLevel) + ",";
  json += "\"presence_detected\":" + String(presenceDetected ? "true" : "false") + ",";
  json += "\"alert_status\":\"" + alertStatus + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void connectToWiFi() {
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("Node 2 IP Address: ");
    Serial.println(WiFi.localIP()); // THIS IS IMPORTANT FOR NODE 3
  } else {
    Serial.println("\nWiFi Connection Failed. Check credentials in arduino_secrets.h");
  }
}
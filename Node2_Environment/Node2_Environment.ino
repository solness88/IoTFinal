/*
 * Node 2 - Environmental Sensor Node
 *
 * Overview:
 * This node is part of a customizable Pomodoro-style focus timer system.
 * It monitors the user's study environment by collecting data from
 * multiple sensors and transmits the data to Node 1 via ESP-NOW every 2 seconds.
 * Sensor readings are also displayed on the OLED display.
 * It also hosts a web dashboard and REST API endpoint via WiFi.
 *
 * Components:
 * - ESP8266 NodeMCU
 * - DHT11 Temperature and Humidity Sensor connected to D4
 * - HC-SR04 Ultrasonic Ranging Module connected to D5(TRIG), D6(ECHO)
 * - Photoresistor connected to A0 (with 10kΩ pull-down resistor)
 * - OLED Display (128x64, I2C, address 0x3C)
 *
 * Pin Connections:
 * - A0: Photoresistor
 * - D4: DHT11 Data
 * - D5: HC-SR04 TRIG
 * - D6: HC-SR04 ECHO
 *
 * Dependencies:
 * - ESP8266WiFi
 * - ESP8266WebServer
 * - espnow
 * - DHT
 * - Wire
 * - Adafruit_GFX
 * - Adafruit_SSD1306
 *
 * Communication:
 * - ESP-NOW: Transmits sensor data to Node 1 every 2 seconds
 * - WiFi: Hosts web dashboard at http://<IP>/ and JSON endpoint at http://<IP>/data
 */

#include <ESP8266WebServer.h>
#include "arduino_secrets.h"
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

ESP8266WebServer server(80);

#define DHTPIN D4
#define DHTTYPE DHT11
#define TRIG_PIN D5
#define ECHO_PIN D6
#define LIGHT_PIN A0
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

DHT dht(DHTPIN, DHTTYPE);

// MAC address of Node 1 (Central Hub) for ESP-NOW communication
uint8_t broadcastAddress[] = {0xC4, 0x5B, 0xBE, 0xF4, 0x2C, 0x67};

// Sensor data structure for ESP-NOW communication with Node 1
// Must match the SensorData structure defined in Node 1
typedef struct struct_message {
  float temp;
  float hum;
  int light;
  float distance;
  int nodeId;
} struct_message;

struct_message myData;

void setup() {
  Serial.begin(115200);

  // Initialize sensors
  dht.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW Init Error");
  return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  Serial.println("Node 2 Started");

  // Connect to WiFi to enable web dashboard and REST API endpoint
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    String html = "<html><body>";
    html += "<h1>Node 2 Dashboard</h1>";
    html += "<p>Temperature: " + String(myData.temp) + " C</p>";
    html += "<p>Humidity: " + String(myData.hum) + " %</p>";
    html += "<p>Light: " + String(myData.light) + "</p>";
    html += "<p>Distance: " + String(myData.distance) + " cm</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/data", []() {
    String json = "{";
    json += "\"temp\":" + String(myData.temp) + ",";
    json += "\"hum\":" + String(myData.hum) + ",";
    json += "\"light\":" + String(myData.light) + ",";
    json += "\"distance\":" + String(myData.distance);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  // End of WiFi and web server setup

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Error");
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void loop() {

  server.handleClient();
  myData.nodeId = 2; 

  // 1. Read all sensor values
  myData.temp = dht.readTemperature();
  myData.hum = dht.readHumidity();
  myData.light = analogRead(LIGHT_PIN);

  // 2. Measure distance using ultrasonic sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  myData.distance = duration * 0.034 / 2;

  // 3. Send sensor data to Node 1 via ESP-NOW
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  // 4. Output sensor values to serial monitor for debugging
  Serial.print("Temp: "); Serial.print(myData.temp);
  Serial.print(" Hum: "); Serial.print(myData.hum);
  Serial.print(" Light: "); Serial.print(myData.light);
  Serial.print(" Dist: "); Serial.println(myData.distance);

  // Sned data in every 2 seconds
  delay(2000);

  // Update OLED display with latest sensor readings
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.print("T:");
  display.print(myData.temp, 1);
  display.println("C");
  display.print("H:");
  display.print(myData.hum, 0);
  display.println("%");
  display.setTextSize(1);
  display.print("Light: ");
  display.println(myData.light);
  display.print("Dist: ");
  display.print(myData.distance, 1);
  display.println("cm");
  display.display();
}
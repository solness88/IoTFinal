/*
 * Node 3 - User Input and Actuator Node
 *
 * Overview:
 * This node is part of a customizable Pomodoro-style focus timer system.
 * It allows the user to set the focus duration using a potentiometer,
 * and sends commands to Node 1 (Central Hub) via ESP-NOW.
 * It receives mode updates from Node 1 and controls actuators accordingly.
 * It also hosts a web dashboard and REST API endpoint via WiFi.
 *
 * Components:
 * - ESP8266 NodeMCU
 * - Potentiometer connected to A0 (sets focus duration 1-60 minutes)
 * - Push Button (Start/Pause) connected to D1
 * - Push Button (Reset) connected to D2
 * - Servo Motor connected to D0
 * - LED Blue connected to D3 (Focus Time indicator)
 * - LED Red connected to D4 (Break Time indicator)
 * - LED Yellow connected to D5 (Ready/Standby indicator)
 * - OLED Display (128x64, I2C, address 0x3C) connected to D6(SDA), D7(SCL)
 *
 * Pin Connections:
 * - A0: Potentiometer
 * - D0: Servo Motor
 * - D1: Start/Pause Button
 * - D2: Reset Button
 * - D3: LED Blue
 * - D4: LED Red
 * - D5: LED Yellow
 * - D6: OLED SDA
 * - D7: OLED SCL
 *
 * Dependencies:
 * - ESP8266WiFi
 * - ESP8266WebServer
 * - espnow
 * - Servo
 * - Wire
 * - Adafruit_GFX
 * - Adafruit_SSD1306
 *
 * Communication:
 * - ESP-NOW: Sends potentiometer value and button commands to Node 1
 *            Receives mode updates (Focus/Break/Ready) from Node 1
 * - WiFi: Hosts web dashboard at http://<IP>/ and JSON endpoint at http://<IP>/data
 */

#include <ESP8266WebServer.h>
#include "arduino_secrets.h"
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

ESP8266WebServer server(80);

// MAC address of Node 1 (Central Hub) for ESP-NOW communication
uint8_t receiverAddress[] = {0xC4, 0x5B, 0xBE, 0xF4, 0x2C, 0x67};

// Pin definitions
const int potPin = A0;
const int btnStart = D1;
const int btnReset = D2;
const int SERVO_PIN = D0;

// LED settings
const int LED_BLUE = D3;
const int LED_RED = D4;
const int LED_YELLOW = D5;

Servo myServo;

// Message structure for ESP-NOW communication with Node 1
// Must match the structure defined in Node 1!!!
struct Message {
  char key;
  int command;
  int potValue;
  int servoCommand;
  int focusMinutes;
};

Message myData;
Message incomingData;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// OLED display update interval
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 200;

// Potentiometer state variables
int lastPotMinutes = 0;

// Current mode received from Node 1
// 1: Break Time, 2: Focus Time, 3: Ready (waiting for input)
int currentServoMode = 3;

int fixedPotMinutes = 1;

// Updates the OLED display with current focus time and mode
void updateDisplay(int minutes) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Focus Timer");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextSize(3);
  display.setCursor(20, 20);
  display.print(minutes);
  display.setTextSize(2);
  display.print(" min");
  display.setTextSize(1);
  display.setCursor(0, 54);

  if (currentServoMode == 1) {
    display.print("Mode: BREAK");
  } else if (currentServoMode == 2) {
    display.print("Mode: FOCUS");
  } else {
    display.print("Mode: READY");
  }
  display.display();
}

// ESP-NOW receive callback
// Receives mode updates from Node 1 and updates currentServoMode
void onDataRecv(uint8_t * mac, uint8_t *data, uint8_t len) {
  memcpy(&incomingData, data, sizeof(incomingData));
  
  // Update mode
  if (incomingData.servoCommand != 0) {
    currentServoMode = incomingData.servoCommand;
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(btnStart, INPUT_PULLUP);
  pinMode(btnReset, INPUT_PULLUP);
  
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);

  // 初期状態
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, HIGH);

  myServo.attach(SERVO_PIN);
  myServo.write(180);
  
  Wire.begin(D6, D7);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 not found");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Initializing...");
    display.display();
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

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
    html += "<h1>Node 3 Dashboard</h1>";
    html += "<p>Focus Time: " + String(fixedPotMinutes) + " min</p>";
    html += "<p>Mode: ";
    if (currentServoMode == 1) html += "BREAK";
    else if (currentServoMode == 2) html += "FOCUS";
    else html += "READY";
    html += "</p></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/data", []() {
    String json = "{";
    json += "\"focusMinutes\":" + String(fixedPotMinutes) + ",";
    json += "\"mode\":";
    if (currentServoMode == 1) json += "\"BREAK\"";
    else if (currentServoMode == 2) json += "\"FOCUS\"";
    else json += "\"READY\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  // End of WiFi and web server setup

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(receiverAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_register_recv_cb(onDataRecv);
  
  Serial.println("Node 3 Ready.");

  lastPotMinutes = map(analogRead(potPin), 0, 1023, 1, 60);
  fixedPotMinutes = lastPotMinutes;
  
}

void loop() {

  server.handleClient();

  // Control actuators (servo, LEDs) based on the mode received from Node 1
  if (currentServoMode == 1) { // Break Time: servo at 0°, red LED on
    myServo.write(0);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, LOW);
  } 
  else if (currentServoMode == 2) { // Focus Time: servo at 180°, blue LED on
    myServo.write(180);
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
  } 
  else if (currentServoMode == 3) { // Ready: servo at 180°, yellow LED on
    myServo.write(180);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, HIGH);
  }

  // Prepare and send data to Node 1 via ESP-NOW
  myData.key = '\0';
  myData.command = 0;
  myData.potValue = 0;
  myData.servoCommand = 0;
  bool shouldSend = false;

  int rawValue = analogRead(potPin);
  int potMinutes = map(rawValue, 0, 1023, 1, 60);

  if (currentServoMode == 3 && abs(potMinutes - lastPotMinutes) >= 1) {
    myData.potValue = potMinutes;
    lastPotMinutes = potMinutes;

    // Only update displayed value in Ready mode
    fixedPotMinutes = potMinutes;
    shouldSend = true;
  }

  if (digitalRead(btnStart) == LOW) {
    myData.command = 2;
    shouldSend = true;
    delay(300);
  }

  if (digitalRead(btnReset) == LOW) {
    myData.command = 1;
    shouldSend = true;
    delay(300);
  }

  if (shouldSend) {
    esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));
  }
  
  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = millis();
    updateDisplay(fixedPotMinutes);
  }

  delay(100);
}
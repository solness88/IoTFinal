/*
 * Node 1 - Central Hub (Focus Timer Controller)
 * 
 * Overview:
 * This system is a customizable Pomodoro-style focus timer that allows
 * users to set their own focus duration using a potentiometer.
 * The timer automatically alternates between Focus Time and Break Time,
 * providing audio and visual feedback throughout the session.
 *
 * This node acts as the central hub of the Focus Timer IoT system.
 * It receives focus time settings and commands from Node 3 via ESP-NOW,
 * receives environmental sensor data from Node 2 via ESP-NOW,
 * manages the focus/break timer, and controls the LCD display,
 * DFPlayer Mini (audio feedback), and active buzzer.
 * It also hosts a web dashboard and REST API endpoint via WiFi.
 *
 * Components:
 * - ESP8266 NodeMCU
 * - LCD Display (16x2, I2C, address 0x27) connected to D1(SCK), D2(SDA)
 * - DFPlayer Mini connected to D7(RX), D8(TX) via 1kΩ resistor on TX
 * - Speaker (UGSM23A 8Ω) connected to DFPlayer SPK1/SPK2
 * - Active Buzzer connected to D3
 *
 * Pin Connections:
 * - D1: LCD SCL
 * - D2: LCD SDA
 * - D3: Active Buzzer
 * - D7: DFPlayer RX
 * - D8: DFPlayer TX (via 1kΩ resistor)
 *
 * Dependencies:
 * - ESP8266WiFi
 * - ESP8266WebServer
 * - espnow
 * - Wire
 * - LiquidCrystal_I2C
 * - SoftwareSerial
 * - DFRobotDFPlayerMini
 *
 * Communication:
 * - ESP-NOW: Receives commands from Node 3, sensor data from Node 2
 * - WiFi: Hosts web dashboard at http://<IP>/ and JSON endpoint at http://<IP>/data
 *
 * Audio Files (SD card):
 * - Track 1: Focus Time BGM
 * - Track 2: Focus Time end notification
 * - Track 3: High temperature alert
 * - Track 4: Low humidity alert
 * - Track 5: Low light alert
 * - Track 6: User away alert
 * - Track 7: User returned notification
 * - Track 8: Break Time BGM
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <espnow.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "arduino_secrets.h"

ESP8266WebServer server(80);

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySoftwareSerial(D7, D8); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// MAC addresses of peer nodes for ESP-NOW communication
uint8_t node3Address[] = {0xDC, 0x4F, 0x22, 0x7F, 0x43, 0xAE};
uint8_t node2Address[] = {0xE8, 0xDB, 0x84, 0xF0, 0x6D, 0xEF};

// Timer state variables
int targetMinutes = 0;
bool isRunning = false;
unsigned long previousMillis = 0;
long remainingSeconds = 0;

// Message structure for ESP-NOW communication with Node 3
// Must match the structure defined in Node 3
struct Message {
  char key;
  int command;
  int potValue;
  int servoCommand;
  int focusMinutes;
};

Message incomingData;

// Sensor data structure for ESP-NOW communication with Node 2
// Must match the structure defined in Node 2
struct SensorData {
  float temp;
  float hum;
  int light;
  float distance;
  int nodeId;
};

// Alert state flags to prevent repeated alerts
bool tempAlertPlayed = false;
bool humAlertPlayed = false;
bool lightAlertPlayed = false;
bool userAway = false;
unsigned long lastAlertTime = 0;
float lastDistance = 0;

bool isBreakTime = false;
const int BREAK_TIME = 5;

// Buzzer settings - sounds every BUZZ_INTERVAL ms during Focus Time
const int BUZZER_PIN = D3;
unsigned long lastBuzzTime = 0;
const unsigned long BUZZ_INTERVAL = 5000;

// Updates the LCD display based on current timer state
void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (isRunning) {
    if (isBreakTime) {
      lcd.print("** BREAK TIME **");
    } else {
      lcd.print("** FOCUSING **");
    }
    lcd.setCursor(0, 1);
    int m = remainingSeconds / 60;
    int s = remainingSeconds % 60;
    lcd.printf("Time: %02d:%02d", m, s);
  } else if (remainingSeconds > 0 && targetMinutes > 0) {
    lcd.print("== PAUSED ==");
    lcd.setCursor(0, 1);
    int m = remainingSeconds / 60;
    int s = remainingSeconds % 60;
    lcd.printf("Time: %02d:%02d", m, s);
  } else if (targetMinutes > 0) {
    lcd.printf("Time: %02d:00", targetMinutes);
    lcd.setCursor(0, 1);
    lcd.print("Blue BTN Start");
  } else {
    lcd.print("Set Focus Time");
    lcd.setCursor(0, 1);
    lcd.print("w/ Potentiometer");
  }
}

// ESP-NOW receive callback
// Distinguishes between Node 2 (SensorData) and Node 3 (Message) by MAC address
void onDataRecv(uint8_t * mac, uint8_t *data, uint8_t len) {

  // If data is from Node 2, process as sensor data
  if (memcmp(mac, node2Address, 6) == 0) {
    Serial.println("SensorData received");
    unsigned long currentTime = millis();
    if (currentTime - lastAlertTime < 2000) return;
    lastAlertTime = currentTime;
    
    SensorData sensorData;
    memcpy(&sensorData, data, sizeof(sensorData));
    
    if (sensorData.temp >= 25.0 && !tempAlertPlayed) {
      myDFPlayer.play(3);
      tempAlertPlayed = true;
    } else if (sensorData.temp < 25.0) {
      tempAlertPlayed = false;
    }
    if (sensorData.hum <= 40.0 && !humAlertPlayed) {
      myDFPlayer.play(4);
      humAlertPlayed = true;
    } else if (sensorData.hum > 40.0) {
      humAlertPlayed = false;
    }
    if (sensorData.light <= 200 && !lightAlertPlayed) {
      myDFPlayer.play(5);
      lightAlertPlayed = true;
    } else if (sensorData.light > 200) {
      lightAlertPlayed = false;
    }
    lastDistance = sensorData.distance;

  // If data is from Node 3, process as command message
  } else {
    memcpy(&incomingData, data, sizeof(incomingData));
    
    if (incomingData.potValue > 0 && !isRunning && incomingData.command == 0 && remainingSeconds == 0) {
      targetMinutes = incomingData.potValue;
      updateDisplay();
    }

    // Blue button: start/pause toggle
    if (incomingData.command == 2) {
      if (targetMinutes > 0 && remainingSeconds == 0) {
        remainingSeconds = targetMinutes * 60;
      }
      if (remainingSeconds > 0) {
        isRunning = !isRunning;
        if (isRunning && remainingSeconds == targetMinutes * 60) {
          myDFPlayer.play(1);
          Message servoMsg;
          servoMsg.key = '\0';
          servoMsg.servoCommand = 2;
          servoMsg.command = 0;
          servoMsg.potValue = 0;
          servoMsg.focusMinutes = targetMinutes;
          esp_now_send(node3Address, (uint8_t *) &servoMsg, sizeof(servoMsg));
        }
        updateDisplay();
      }

    // Red button: reset timer and return to ready state  
    } else if (incomingData.command == 1) {
      isRunning = false;
      isBreakTime = false;
      targetMinutes = 0;
      remainingSeconds = 0;
      myDFPlayer.stop();
      Message servoMsg;
      servoMsg.key = '\0';
      servoMsg.servoCommand = 3;
      servoMsg.command = 0;
      servoMsg.potValue = 0;
      esp_now_send(node3Address, (uint8_t *) &servoMsg, sizeof(servoMsg));
      updateDisplay();
    }
  }
}

void setup() {
  Serial.begin(115200);

  delay(2000);
  Serial.print("sizeof(Message): ");
  Serial.println(sizeof(Message));
  Serial.print("sizeof(SensorData): ");
  Serial.println(sizeof(SensorData));


  mySoftwareSerial.begin(9600);
  delay(1000);

  lcd.init();
  lcd.backlight();

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Error");
    lcd.clear();
    lcd.print("DFPlayer Error");
    //while(1);
  }

  delay(500);
  myDFPlayer.volume(20);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  updateDisplay();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) return;
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);

  esp_now_add_peer(node3Address, ESP_NOW_ROLE_CONTROLLER, 1, NULL, 0);
  Message initMsg;
  initMsg.key = '\0';
  initMsg.servoCommand = 3;  //Wait for input
  initMsg.command = 0;
  initMsg.potValue = 0;
  esp_now_send(node3Address, (uint8_t *) &initMsg, sizeof(initMsg));

  Serial.print("sizeof(Message): ");
  Serial.println(sizeof(Message));
  Serial.print("sizeof(SensorData): ");
  Serial.println(sizeof(SensorData));

  // WiFi.begin(SECRET_SSID, SECRET_PASS);

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.print("IP: ");
  // Serial.println(WiFi.localIP());

  // server.on("/", []() {
  //   String html = "<html><body>";
  //   html += "<h1>Node 1 Dashboard</h1>";
  //   html += "<p>Status: " + String(isRunning ? "Running" : "Stopped") + "</p>";
  //   html += "<p>Target: " + String(targetMinutes) + " min</p>";
  //   html += "<p>Remaining: " + String(remainingSeconds / 60) + " min " + String(remainingSeconds % 60) + " sec</p>";
  //   html += "<p>Mode: " + String(isBreakTime ? "Break" : "Focus") + "</p>";
  //   html += "</body></html>";
  //   server.send(200, "text/html", html);
  // });

  // server.on("/data", []() {
  //   String json = "{";
  //   json += "\"status\":\"" + String(isRunning ? "running" : "stopped") + "\",";
  //   json += "\"targetMinutes\":" + String(targetMinutes) + ",";
  //   json += "\"remainingSeconds\":" + String(remainingSeconds) + ",";
  //   json += "\"isBreakTime\":" + String(isBreakTime ? "true" : "false");
  //   json += "}";
  //   server.send(200, "application/json", json);
  // });

  // server.begin();
}

void loop() {

  // server.handleClient();

  // Absence detection - only active during Focus Time (not Break Time)
  if (!isBreakTime) {
    if (lastDistance >= 80.0 && !userAway) {
      myDFPlayer.play(6);
      userAway = true;
            if (isRunning) {
        isRunning = false;
        updateDisplay();
      }
    } else if (lastDistance <= 50.0 && userAway) {
      myDFPlayer.play(7);
      userAway = false;
      if (remainingSeconds > 0) {
        isRunning = true;
        updateDisplay();
      }
    }
  }

  // Timer processing - count down every second
  if (isRunning && remainingSeconds > 0) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000) {
      previousMillis = currentMillis;
      remainingSeconds--;

    if (!isBreakTime) {
      unsigned long currentMillis2 = millis();
      if (currentMillis2 - lastBuzzTime >= BUZZ_INTERVAL) {
        lastBuzzTime = currentMillis2;
        digitalWrite(BUZZER_PIN, HIGH);
        delay(50);
        digitalWrite(BUZZER_PIN, LOW);
      }
    }

    updateDisplay();

      // Timer reached zero - switch between Focus and Break Time
      if (remainingSeconds <= 0) {
        if (!isBreakTime) {
          // Update "isBreakTime" and "remainingSeconds"
          isBreakTime = true;
          remainingSeconds = BREAK_TIME * 60;
          isRunning = true;
          
          // Servo and voice
          Message servoMsg;
          servoMsg.key = '\0';
          servoMsg.servoCommand = 1;
          servoMsg.command = 0;
          servoMsg.potValue = 0;

          esp_now_send(node3Address, (uint8_t *) &servoMsg, sizeof(servoMsg));
          
          myDFPlayer.play(2);
          delay(3000);
          myDFPlayer.play(8);
          lcd.clear();
          lcd.print("Break Time!");
          delay(2000);
        } else {
          // Update "isBreakTime" and "remainingSeconds"
          isBreakTime = false;
          remainingSeconds = targetMinutes * 60;
          isRunning = true;
          
          // Servo and voice
          Message servoMsg;
          servoMsg.key = '\0';
          servoMsg.servoCommand = 2;
          servoMsg.command = 0;
          servoMsg.potValue = 0;
          esp_now_send(node3Address, (uint8_t *) &servoMsg, sizeof(servoMsg));
          
          myDFPlayer.stop();
          delay(500);
          myDFPlayer.play(1);
          lcd.clear();
          lcd.print("Study Time!");
          delay(2000);
        }
        updateDisplay();
      }
    }
  }
}
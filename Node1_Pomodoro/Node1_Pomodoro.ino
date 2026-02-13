/*
 * Smart Study Environment System - Node 1
 * Pomodoro Timer with Environment Monitoring + HTTP Server
 * 
 * Hardware:
 * - ESP8266 NodeMCU
 * - DHT11 (Temperature & Humidity)
 * - LCD I2C (16x2)
 * - Push Buttons x3
 * - LEDs (Red, Green)
 * - Active Buzzer
 */

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "arduino_secrets.h"  // Load WiFi credentials from the secret tab

// ===== WiFi Configuration =====
const char* WIFI_SSID = SECRET_SSID;      // Replace with your WiFi SSID
const char* WIFI_PASSWORD = SECRET_PASS;  // Replace with your WiFi password

// ===== Pin Definitions =====
#define DHT_PIN D4          // DHT11 sensor
#define BUTTON_START D5     // Start/Pause button
#define BUTTON_RESET D6     // Reset button
#define BUTTON_CONFIG D7    // Config button (for future use)
#define LED_GREEN D3        // Green LED (timer running)
#define LED_RED D0          // Red LED (timer stopped)
#define BUZZER_PIN D8       // Buzzer

// ===== Sensor Initialization =====
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16x2 display

// ===== HTTP Server =====
ESP8266WebServer server(80);  // HTTP server on port 80

// ===== Timer Configuration =====
// const unsigned long POMODORO_DURATION = 25 * 60 * 1000;  // 25 minutes (milliseconds)
// For testing, use shorter duration:
const unsigned long POMODORO_DURATION = 10 * 1000; // 10 seconds

unsigned long timerStartTime = 0;
unsigned long timerPausedTime = 0;
unsigned long timerElapsed = 0;
bool timerRunning = false;
bool timerCompleted = false;

// ===== Button State Management (Debouncing) =====
unsigned long lastDebounceTime[3] = {0, 0, 0};
const unsigned long debounceDelay = 200;  // 200ms
bool lastButtonState[3] = {HIGH, HIGH, HIGH};

// ===== Environment Data Update Interval =====
unsigned long lastDHTRead = 0;
const unsigned long DHT_INTERVAL = 2000;  // Every 2 seconds
float temperature = 0;
float humidity = 0;

// ===== Buzzer Alarm Control =====
unsigned long alarmStartTime = 0;
const unsigned long ALARM_DURATION = 3000;  // Sound for 3 seconds
bool alarmActive = false;

void setup() {
  Serial.begin(9600);
  
  // Pin mode setup
  pinMode(BUTTON_START, INPUT_PULLUP);
  pinMode(BUTTON_RESET, INPUT_PULLUP);
  pinMode(BUTTON_CONFIG, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initial state: Red LED on (stopped)
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Sensor initialization
  dht.begin();
  lcd.init();
  lcd.backlight();
  
  // Startup message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Study");
  lcd.setCursor(0, 1);
  lcd.print("System Ready!");
  delay(2000);
  
  // WiFi connection
  connectToWiFi();
  
  // HTTP server endpoints
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  
  lcd.clear();
  Serial.println("=== Smart Study Environment System ===");
  Serial.println("Node 1: Pomodoro Timer + Environment Monitor");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // 1. Handle HTTP requests
  server.handleClient();
  
  // 2. Read environment data
  readEnvironmentData();
  
  // 3. Handle button inputs
  handleButtons();
  
  // 4. Update timer
  updateTimer();
  
  // 5. Update LCD display
  updateDisplay();
  
  // 6. Handle alarm
  handleAlarm();
  
  delay(100);  // Main loop delay
}

// ===== WiFi Connection =====
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connecting");
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(attempts % 16, 1);
    lcd.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(3000);
  } else {
    Serial.println("\nWiFi Connection Failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    delay(2000);
  }
}

// ===== HTTP Handler: Root =====
void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'><title>Node 1 - Pomodoro Timer</title></head>";
  html += "<body><h1>Node 1: Pomodoro Timer</h1>";
  html += "<p>Temperature: " + String(temperature) + " °C</p>";
  html += "<p>Humidity: " + String(humidity) + " %</p>";
  html += "<p>Timer Status: " + getTimerStatus() + "</p>";
  html += "<p>Timer Remaining: " + getTimerRemaining() + "</p>";
  html += "<p><a href='/data'>View JSON Data</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// ===== HTTP Handler: Data Endpoint (JSON) =====
void handleData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 0) + ",";
  json += "\"timer_remaining\":\"" + getTimerRemaining() + "\",";
  json += "\"status\":\"" + getTimerStatus() + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
  Serial.println("Data request served: " + json);
}

// ===== Get Timer Status (String) =====
String getTimerStatus() {
  if (timerCompleted) {
    return "completed";
  } else if (timerRunning) {
    return "studying";
  } else if (timerElapsed > 0) {
    return "paused";
  } else {
    return "ready";
  }
}

// ===== Get Timer Remaining (String MM:SS) =====
String getTimerRemaining() {
  unsigned long remaining = POMODORO_DURATION - timerElapsed;
  int minutes = remaining / 60000;
  int seconds = (remaining % 60000) / 1000;
  
  String result = "";
  if (minutes < 10) result += "0";
  result += String(minutes);
  result += ":";
  if (seconds < 10) result += "0";
  result += String(seconds);
  
  return result;
}

// ===== Read Environment Data =====
void readEnvironmentData() {
  if (millis() - lastDHTRead >= DHT_INTERVAL) {
    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();
    
    if (!isnan(newTemp) && !isnan(newHum)) {
      temperature = newTemp;
      humidity = newHum;
      
      Serial.print("Temp: ");
      Serial.print(temperature);
      Serial.print("C, Hum: ");
      Serial.print(humidity);
      Serial.println("%");
    }
    
    lastDHTRead = millis();
  }
}

// ===== Handle Button Inputs =====
void handleButtons() {
  // Button 1: Start/Pause
  if (checkButton(0, BUTTON_START)) {
    if (!timerRunning && !timerCompleted) {
      // Start or resume timer
      if (timerElapsed == 0) {
        // New start
        timerStartTime = millis();
      } else {
        // Resume from pause
        timerStartTime = millis() - timerElapsed;
      }
      timerRunning = true;
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
      Serial.println("Timer STARTED");
    } else if (timerRunning) {
      // Pause
      timerElapsed = millis() - timerStartTime;
      timerRunning = false;
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      Serial.println("Timer PAUSED");
    }
  }
  
  // Button 2: Reset
  if (checkButton(1, BUTTON_RESET)) {
    timerStartTime = 0;
    timerElapsed = 0;
    timerRunning = false;
    timerCompleted = false;
    alarmActive = false;
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Timer RESET");
  }
  
  // Button 3: Config (for future expansion)
  if (checkButton(2, BUTTON_CONFIG)) {
    Serial.println("Config button pressed (not implemented)");
  }
}

// ===== Button Debouncing Check =====
bool checkButton(int buttonIndex, int buttonPin) {
  bool currentState = digitalRead(buttonPin);
  
  if (currentState == LOW && lastButtonState[buttonIndex] == HIGH) {
    if (millis() - lastDebounceTime[buttonIndex] > debounceDelay) {
      lastDebounceTime[buttonIndex] = millis();
      lastButtonState[buttonIndex] = LOW;
      return true;
    }
  }
  
  if (currentState == HIGH) {
    lastButtonState[buttonIndex] = HIGH;
  }
  
  return false;
}

// ===== Update Timer =====
void updateTimer() {
  if (timerRunning) {
    timerElapsed = millis() - timerStartTime;
    
    // Check if 25 minutes elapsed
    if (timerElapsed >= POMODORO_DURATION && !timerCompleted) {
      timerCompleted = true;
      timerRunning = false;
      alarmActive = true;
      alarmStartTime = millis();
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      Serial.println("=== POMODORO COMPLETED! ===");
    }
  }
}

// ===== Update LCD Display =====
void updateDisplay() {
  // Top line: Temperature and Humidity
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C H:");
  lcd.print(humidity, 0);
  lcd.print("%  ");
  
  // Bottom line: Timer
  lcd.setCursor(0, 1);
  
  if (timerCompleted) {
    lcd.print("COMPLETE!       ");
  } else {
    unsigned long remaining = POMODORO_DURATION - timerElapsed;
    int minutes = remaining / 60000;
    int seconds = (remaining % 60000) / 1000;
    
    // Timer status display
    if (timerRunning) {
      lcd.print("RUN ");
    } else if (timerElapsed > 0) {
      lcd.print("PAUSE ");
    } else {
      lcd.print("READY ");
    }
    
    // Remaining time display
    if (minutes < 10) lcd.print("0");
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10) lcd.print("0");
    lcd.print(seconds);
    lcd.print("    ");
  }
}

// ===== Handle Alarm =====
void handleAlarm() {
  if (alarmActive) {
    if (millis() - alarmStartTime < ALARM_DURATION) {
      // Make buzzer beep (500ms ON, 500ms OFF)
      if ((millis() / 500) % 2 == 0) {
        digitalWrite(BUZZER_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
      }
    } else {
      // End alarm
      digitalWrite(BUZZER_PIN, LOW);
      alarmActive = false;
    }
  }
}
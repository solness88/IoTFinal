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
uint8_t broadcastAddress[] = {0xC4, 0x5B, 0xBE, 0xF4, 0x2C, 0x67};

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



Serial.print("MAC: ");
Serial.println(WiFi.macAddress());





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

  // 1. 各センサーの値を読み取る
  myData.temp = dht.readTemperature();
  myData.hum = dht.readHumidity();
  myData.light = analogRead(LIGHT_PIN);

  // 2. 超音波センサーで距離を測定
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  myData.distance = duration * 0.034 / 2;

  // 3. データを送信！(これが重要)
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  // 4. 確認用にシリアルモニターに出力
  Serial.print("Temp: "); Serial.print(myData.temp);
  Serial.print(" Hum: "); Serial.print(myData.hum);
  Serial.print(" Light: "); Serial.print(myData.light);
  Serial.print(" Dist: "); Serial.println(myData.distance);

  delay(2000); // 2秒おきに送信

  // OLED表示
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
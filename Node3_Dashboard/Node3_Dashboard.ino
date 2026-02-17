#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>

uint8_t receiverAddress[] = {0xC4, 0x5B, 0xBE, 0xF4, 0x2C, 0x67};

const int potPin = A0;
const int btnStart = D1;
const int btnReset = D2;
const int SERVO_PIN = D0;

Servo myServo;

struct Message {
  char key;
  int command;
  int potValue;
  int servoCommand;
};
Message myData;
Message incomingData;

int lastPotMinutes = 0;

void onDataRecv(uint8_t * mac, uint8_t *data, uint8_t len) {
  memcpy(&incomingData, data, sizeof(incomingData));
    Serial.print("servoCommand: ");
  Serial.println(incomingData.servoCommand);  // ← 追加
  if (incomingData.servoCommand == 1) {
    myServo.write(0);  // バナー表示
  } else if (incomingData.servoCommand == 2) {
    myServo.write(180);    // バナー隠す
  }
}
void setup() {
  Serial.begin(115200);
  
  pinMode(btnStart, INPUT_PULLUP);
  pinMode(btnReset, INPUT_PULLUP);
  
  myServo.attach(SERVO_PIN);
  myServo.write(180);  // 初期位置
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(receiverAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_register_recv_cb(onDataRecv);
  
  Serial.println("Node 3 Ready.");
  Serial.println(WiFi.macAddress());
}

void loop() {
  myData.key = '\0';
  myData.command = 0;
  myData.potValue = 0;
  myData.servoCommand = 0;
  bool shouldSend = false;

  // ポテンショメーター読み取り（0-1023を0-60分に変換）
  int rawValue = analogRead(potPin);
  int potMinutes = map(rawValue, 0, 1023, 0, 60);

  // 値が変わった時だけ送信
  if (potMinutes != lastPotMinutes) {
    myData.potValue = potMinutes;
    lastPotMinutes = potMinutes;
    shouldSend = true;
    Serial.print("Minutes: ");
    Serial.println(potMinutes);
  }

  // スタート/一時停止ボタン
  if (digitalRead(btnStart) == LOW) {
    myData.command = 2;
    shouldSend = true;
    Serial.println("Command: Start/Pause");
    delay(300);
  }

  // リセットボタン
  if (digitalRead(btnReset) == LOW) {
    myData.command = 1;
    shouldSend = true;
    Serial.println("Command: Reset");
    delay(300);
  }

  if (shouldSend) {
    esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));
  }
  
  delay(100);
}
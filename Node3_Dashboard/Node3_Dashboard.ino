// #include <ESP8266WiFi.h>
// #include <espnow.h>
// #include <Servo.h>

// uint8_t receiverAddress[] = {0xC4, 0x5B, 0xBE, 0xF4, 0x2C, 0x67};

// const int potPin = A0;
// const int btnStart = D1;
// const int btnReset = D2;
// const int SERVO_PIN = D0;

// // LED color settings
// const int LED_BLUE = D3;
// const int LED_RED = D4;
// const int LED_YELLOW = D5;

// // Motor settings
// const int MOTOR_PIN = D8;

// Servo myServo;

// struct Message {
//   char key;
//   int command;
//   int potValue;
//   int servoCommand;
// };
// Message myData;
// Message incomingData;

// int lastPotMinutes = 0;

// void onDataRecv(uint8_t * mac, uint8_t *data, uint8_t len) {
//   memcpy(&incomingData, data, sizeof(incomingData));
//   Serial.print("servoCommand: ");
//   Serial.println(incomingData.servoCommand);
  
//   // if (incomingData.servoCommand == 1) {  // Break Time
//   //   myServo.write(0);
//   //   digitalWrite(LED_BLUE, LOW);
//   //   digitalWrite(LED_RED, HIGH);
//   //   digitalWrite(LED_YELLOW, LOW);
//   // } else if (incomingData.servoCommand == 2) {  // Focus Time
//   //   myServo.write(180);
//   //   digitalWrite(LED_BLUE, HIGH);
//   //   digitalWrite(LED_RED, LOW);
//   //   digitalWrite(LED_YELLOW, LOW);
//   // } else if (incomingData.servoCommand == 3) {  // 入力待ち
//   //   myServo.write(180);
//   //   digitalWrite(LED_BLUE, LOW);
//   //   digitalWrite(LED_RED, LOW);
//   //   digitalWrite(LED_YELLOW, HIGH);
//   // }
//   if (incomingData.servoCommand == 1) {  // Break Time
//     myServo.write(0);
//     delay(1000);  // サーボが動き終わるまで待つ
//     digitalWrite(LED_BLUE, LOW);
//     digitalWrite(LED_RED, HIGH);
//     digitalWrite(LED_YELLOW, LOW);
//     digitalWrite(MOTOR_PIN, HIGH);  // 0-1023の範囲で速度調整（512=半速）
//   } else if (incomingData.servoCommand == 2) {  // Focus Time
//     myServo.write(180);
//     digitalWrite(LED_BLUE, HIGH);
//     digitalWrite(LED_RED, LOW);
//     digitalWrite(LED_YELLOW, LOW);
//     digitalWrite(MOTOR_PIN, LOW);  // モーター停止
//   } else if (incomingData.servoCommand == 3) {  // 入力待ち
//     myServo.write(180);
//     digitalWrite(LED_BLUE, LOW);
//     digitalWrite(LED_RED, LOW);
//     digitalWrite(LED_YELLOW, HIGH);
//     digitalWrite(MOTOR_PIN, LOW);  // モーター停止
//   }
// }

// void setup() {
//   Serial.begin(115200);
  
//   pinMode(btnStart, INPUT_PULLUP);
//   pinMode(btnReset, INPUT_PULLUP);
  
//   pinMode(LED_BLUE, OUTPUT);
//   pinMode(LED_RED, OUTPUT);
//   pinMode(LED_YELLOW, OUTPUT);
  
//   // 初期は黄色点灯
//   digitalWrite(LED_BLUE, LOW);
//   digitalWrite(LED_RED, LOW);
//   digitalWrite(LED_YELLOW, HIGH);

//   myServo.attach(SERVO_PIN);
//   myServo.write(180);
  
//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();
  
//   if (esp_now_init() != 0) {
//     Serial.println("ESP-NOW Init Failed");
//     return;
//   }
  
//   esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
//   esp_now_add_peer(receiverAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
//   esp_now_register_recv_cb(onDataRecv);
  
//   Serial.println("Node 3 Ready.");
//   Serial.println(WiFi.macAddress());

//   pinMode(MOTOR_PIN, OUTPUT);

// }

// void loop() {
//   myData.key = '\0';
//   myData.command = 0;
//   myData.potValue = 0;
//   myData.servoCommand = 0;
//   bool shouldSend = false;

//   int rawValue = analogRead(potPin);
//   int potMinutes = map(rawValue, 0, 1023, 0, 60);

//   if (potMinutes != lastPotMinutes) {
//     myData.potValue = potMinutes;
//     lastPotMinutes = potMinutes;
//     shouldSend = true;
//     Serial.print("Minutes: ");
//     Serial.println(potMinutes);
//   }

//   if (digitalRead(btnStart) == LOW) {
//     myData.command = 2;
//     shouldSend = true;
//     Serial.println("Command: Start/Pause");
//     delay(300);
//   }

//   if (digitalRead(btnReset) == LOW) {
//     myData.command = 1;
//     shouldSend = true;
//     Serial.println("Command: Reset");
//     delay(300);
//   }

//   if (shouldSend) {
//     esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));
//   }
  
//   delay(100);
// }






#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>

// Node 1のMACアドレス
uint8_t receiverAddress[] = {0xC4, 0x5B, 0xBE, 0xF4, 0x2C, 0x67};

const int potPin = A0;
const int btnStart = D1;
const int btnReset = D2;
const int SERVO_PIN = D0;

// LED設定
const int LED_BLUE = D3;
const int LED_RED = D4;
const int LED_YELLOW = D5;

// モーター設定
const int MOTOR_PIN = D6;

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
int currentServoMode = 3; // 受信したモードを保持する変数（初期値は入力待ち）

void onDataRecv(uint8_t * mac, uint8_t *data, uint8_t len) {
  memcpy(&incomingData, data, sizeof(incomingData));
  Serial.print("Received servoCommand: ");
  Serial.println(incomingData.servoCommand);
  
  // モードを更新
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
  pinMode(MOTOR_PIN, OUTPUT);

  // 初期状態
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, HIGH);
  digitalWrite(MOTOR_PIN, LOW);

  myServo.attach(SERVO_PIN);
  myServo.write(180);
  
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



  
digitalWrite(MOTOR_PIN, LOW);
}

void loop() {
  // --- 受信したモードに基づいてアクチュエータ（サーボ・LED・モーター）を制御 ---
  if (currentServoMode == 1) {  // Break Time
    myServo.write(0);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(MOTOR_PIN, HIGH); // モーター回転
  } 
  else if (currentServoMode == 2) {  // Focus Time
    myServo.write(180);
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(MOTOR_PIN, LOW);  // モーター停止
  } 
  else if (currentServoMode == 3) {  // 入力待ち
    myServo.write(180);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(MOTOR_PIN, LOW);  // モーター停止
  }

  // --- 送信処理 ---
  myData.key = '\0';
  myData.command = 0;
  myData.potValue = 0;
  myData.servoCommand = 0;
  bool shouldSend = false;

  int rawValue = analogRead(potPin);
  int potMinutes = map(rawValue, 0, 1023, 0, 60);

  if (abs(potMinutes - lastPotMinutes) >= 1) {
    myData.potValue = potMinutes;
    lastPotMinutes = potMinutes;
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
  
  delay(100);
}
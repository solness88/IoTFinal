#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySoftwareSerial(D7, D8); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

int targetMinutes = 0;
bool isRunning = false;
unsigned long previousMillis = 0;
long remainingSeconds = 0;

struct Message {
  char key;
  int command;
  int potValue;
};

Message incomingData;

struct SensorData {
  float temp;
  float hum;
  int light;
  float distance;
};

bool tempAlertPlayed = false;
bool humAlertPlayed = false;
bool lightAlertPlayed = false;
bool userAway = false;
unsigned long lastAlertTime = 0;
float lastDistance = 0;

bool isBreakTime = false;
const int BREAK_TIME = 5;   // 休憩時間（分）


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
  } else if (remainingSeconds > 0 && targetMinutes > 0) {  // ← PAUSE表示追加
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

void onDataRecv(uint8_t * mac, uint8_t *data, uint8_t len) {
  // Node3からのメッセージ（サイズ判定）
  if (len == sizeof(Message)) {
    memcpy(&incomingData, data, sizeof(incomingData));
    
    // ポテンショメーター値受信
    if (incomingData.potValue > 0 && !isRunning && incomingData.command == 0 && remainingSeconds == 0) {
      targetMinutes = incomingData.potValue;
      //remainingSeconds = targetMinutes * 60;
      updateDisplay();
    }
    
    // D2ボタン（青）：開始と一時停止のトグル
    if (incomingData.command == 2) {
      if (targetMinutes > 0 && remainingSeconds == 0) {
        remainingSeconds = targetMinutes * 60;  // ← ここで初めて設定
      }
      if (remainingSeconds > 0) {
        isRunning = !isRunning;
        if (isRunning && remainingSeconds == targetMinutes * 60) {
          myDFPlayer.play(1);
        }
        updateDisplay();
      }
    }
    
    // D0ボタン（赤）：リセット
    else if (incomingData.command == 1) {
      isRunning = false;
      targetMinutes = 0;
      remainingSeconds = 0;
      myDFPlayer.stop();
      updateDisplay();
    }
  }
  
  // Node2からのセンサーデータ
  else if (len == sizeof(SensorData)) {
    unsigned long currentTime = millis();
    if (currentTime - lastAlertTime < 2000) {  // 2秒以内ならスキップ
      return;
    }
    lastAlertTime = currentTime;
    
    SensorData sensorData;
    memcpy(&sensorData, data, sizeof(sensorData));
    
    // 1. 温度チェック (25度以上)
    if (sensorData.temp >= 25.0 && !tempAlertPlayed) {
      myDFPlayer.play(3);
      tempAlertPlayed = true;
    } else if (sensorData.temp < 25.0) {
      tempAlertPlayed = false;
    }
    
    // 2. 湿度チェック (40%以下)
    if (sensorData.hum <= 40.0 && !humAlertPlayed) {
      myDFPlayer.play(4);
      humAlertPlayed = true;
    } else if (sensorData.hum > 40.0) {
      humAlertPlayed = false;
    }
    
    // 3. 明るさチェック (200以下)
    if (sensorData.light <= 200 && !lightAlertPlayed) {
      myDFPlayer.play(5);
      lightAlertPlayed = true;
    } else if (sensorData.light > 200) {
      lightAlertPlayed = false;
    }
    
    // 4&5. 離席検知 - データ保存のみ
    lastDistance = sensorData.distance;
  }
}

void setup() {
  Serial.begin(115200);
  mySoftwareSerial.begin(9600);
  delay(1000);

  lcd.init();
  lcd.backlight();

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Error");
    lcd.clear();
    lcd.print("DFPlayer Error");
    while(1);
  }
  Serial.println("DFPlayer OK");
  delay(500);
  myDFPlayer.volume(30);
  
  updateDisplay();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) return;
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);
}

void loop() {
  // 離席検知処理（学習中のみ）
  if (!isBreakTime) {  // ← 追加
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
  }  // ← 追加

  // タイマー処理（以下変更なし）
  if (isRunning && remainingSeconds > 0) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000) {
      previousMillis = currentMillis;
      remainingSeconds--;
      updateDisplay();

      if (remainingSeconds <= 0) {
        if (!isBreakTime) {
          isBreakTime = true;
          remainingSeconds = BREAK_TIME * 60;
          isRunning = true;
          myDFPlayer.play(2);
          lcd.clear();
          lcd.print("Break Time!");
          delay(2000);
        } else {
          isBreakTime = false;
          remainingSeconds = targetMinutes * 60;
          isRunning = true;
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
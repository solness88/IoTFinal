#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySoftwareSerial(D7, D8); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

uint8_t node3Address[] = {0xDC, 0x4F, 0x22, 0x7F, 0x43, 0xAE};

int targetMinutes = 0;
bool isRunning = false;
unsigned long previousMillis = 0;
long remainingSeconds = 0;

struct Message {
  char key;
  int command;
  int potValue;
  int servoCommand;
    int focusMinutes;/////////
};

Message incomingData;

struct SensorData {
  float temp;
  float hum;
  int light;
  float distance;
  int nodeId;
};

bool tempAlertPlayed = false;
bool humAlertPlayed = false;
bool lightAlertPlayed = false;
bool userAway = false;
unsigned long lastAlertTime = 0;
float lastDistance = 0;

bool isBreakTime = false;
const int BREAK_TIME = 5;   // 休憩時間（分）


const int BUZZER_PIN = D3;
unsigned long lastBuzzTime = 0;
const unsigned long BUZZ_INTERVAL = 5000;

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

    Serial.print("len: ");
  Serial.println(len);  // ← 追加




  // Node3からのメッセージ（サイズ判定）
  if (len == sizeof(Message)) {


    Serial.println("Message received");  // ← 追加

Serial.print("potValue: ");
Serial.println(incomingData.potValue);
Serial.print("command: ");
Serial.println(incomingData.command);


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
    remainingSeconds = targetMinutes * 60;
  }
  if (remainingSeconds > 0) {
    isRunning = !isRunning;
    if (isRunning && remainingSeconds == targetMinutes * 60) {
      // Focus Time開始
      myDFPlayer.play(1);
      
      Message servoMsg;
      servoMsg.key = '\0';
      servoMsg.servoCommand = 2;
      servoMsg.command = 0;
      servoMsg.potValue = 0;
    servoMsg.focusMinutes = targetMinutes;  //////////////////////      
      esp_now_send(node3Address, (uint8_t *) &servoMsg, sizeof(servoMsg));
    }
    updateDisplay();
  }
}
    
    // D0ボタン（赤）：リセット
    else if (incomingData.command == 1) {
  isRunning = false;
  isBreakTime = false;
  targetMinutes = 0;
  remainingSeconds = 0;
  myDFPlayer.stop();
  
  Message servoMsg;
  servoMsg.key = '\0';
  servoMsg.servoCommand = 3;  // ← 入力待ちモード
  servoMsg.command = 0;
  servoMsg.potValue = 0;
  esp_now_send(node3Address, (uint8_t *) &servoMsg, sizeof(servoMsg));
  
  updateDisplay();
    }
  }
  
  // Node2からのセンサーデータ
  else if (len == sizeof(SensorData)) {



    Serial.println("SensorData received");  // ← 追加





    unsigned long currentTime = millis();
    if (currentTime - lastAlertTime < 2000) {  // 2秒以内ならスキップ
      return;
    }
    lastAlertTime = currentTime;
    
    SensorData sensorData;
    memcpy(&sensorData, data, sizeof(sensorData));
      
    Serial.print("Temp: "); Serial.println(sensorData.temp);
    Serial.print("Hum: "); Serial.println(sensorData.hum);
    Serial.print("Light: "); Serial.println(sensorData.light);
    Serial.print("Dist: "); Serial.println(sensorData.distance);

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


  delay(2000);  // ← 変更
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
  //Serial.println("DFPlayer OK");
  delay(500);
  myDFPlayer.volume(30);
  


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
initMsg.servoCommand = 3;  // ← 入力待ちモード
initMsg.command = 0;
initMsg.potValue = 0;
esp_now_send(node3Address, (uint8_t *) &initMsg, sizeof(initMsg));

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

if (remainingSeconds <= 0) {
  if (!isBreakTime) {
    // すぐにisBreakTimeとremainingSecondsを更新
    isBreakTime = true;
    remainingSeconds = BREAK_TIME * 60;
    isRunning = true;
    
    // その後サーボと音声
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
    // すぐにisBreakTimeとremainingSecondsを更新
    isBreakTime = false;
    remainingSeconds = targetMinutes * 60;
    isRunning = true;
    
    // その後サーボと音声
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
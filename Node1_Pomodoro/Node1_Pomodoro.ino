#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// ピン・デバイス設定
SoftwareSerial mySoftwareSerial(D7, D8); 
LiquidCrystal_I2C lcd(0x27, 16, 2);
DFRobotDFPlayerMini myDFPlayer;

// 受信データの構造体
typedef struct struct_message {
    float temp;
    float hum;
    int light;
    float distance;
} struct_message;

struct_message myData;

// 再生管理フラグ（同じ音を何度も鳴らさないため）
unsigned long lastPlayTime = 0;
const int playInterval = 8000; // 次の音声まで8秒待機
bool isAway = false;          // 離席状態の管理

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  
  // 液晶表示
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(myData.temp, 1);
  lcd.print(" H:"); lcd.print(myData.hum, 0);
  lcd.print(" L:"); lcd.print(myData.light);
  lcd.setCursor(0, 1);
  lcd.print("D:"); lcd.print(myData.distance, 1);
  lcd.print("cm      ");

  // --- 音声再生ロジック ---
  if (millis() - lastPlayTime > playInterval) {

    // 1. 離席判定（距離が一定以上離れたら：例 80cm以上）
    if (myData.distance > 80.0 && !isAway) {
      myDFPlayer.play(6); // Away detected.
      isAway = true;
      lastPlayTime = millis();
    }
    // 2. 戻り判定（距離が近くなったら：例 30cm以内）
    else if (myData.distance < 30.0 && isAway) {
      myDFPlayer.play(7); // Welcome back!
      isAway = false;
      lastPlayTime = millis();
    }
    // 3. 暗さ判定（光センサ 150未満）
    else if (myData.light < 150) {
      myDFPlayer.play(5); // It's too dark.
      lastPlayTime = millis();
    }
    // 4. 高温判定（30度以上）
    else if (myData.temp > 30.0) {
      myDFPlayer.play(3); // It's too hot.
      lastPlayTime = millis();
    }
    // 5. 低湿度判定（40%未満）
    else if (myData.hum < 40.0) {
      myDFPlayer.play(4); // Low humidity.
      lastPlayTime = millis();
    }
  }
}

void setup() {
  Serial.begin(115200);
  mySoftwareSerial.begin(9600);
  
  lcd.init();
  lcd.backlight();
  lcd.print("System Ready");

  delay(2000); // DFPlayer安定待ち
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Error");
  } else {
    myDFPlayer.volume(22); // 適正音量
    Serial.println("DFPlayer OK");
  }

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) return;
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
}
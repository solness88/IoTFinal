// #include <SoftwareSerial.h>
// #include <DFRobotDFPlayerMini.h>
// #include <ESP8266WiFi.h>
// #include <espnow.h>
// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>

// // DFPlayer用ピン設定（D7/D8に変更）
// SoftwareSerial mySoftwareSerial(D7, D8); // RX, TX
// DFRobotDFPlayerMini myDFPlayer;

// // LCD設定 (I2Cアドレス 0x27, 16x2サイズ)
// LiquidCrystal_I2C lcd(0x27, 16, 2);

// // ESP-NOW用データ構造体（Node2と同じ）
// typedef struct struct_message {
//   float temp;
//   float hum;
//   int light;
//   float distance;
// } struct_message;

// struct_message receivedData;

// // しきい値設定
// const int DARK_THRESHOLD = 300;      // 暗いと判断する照度
// const float AWAY_THRESHOLD = 100.0;  // 人がいないと判断する距離(cm)

// bool isDark = false;
// bool isAway = false;
// bool alreadyPlayed = false;

// // ESP-NOWでデータ受信時に呼ばれる関数
// void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
//   memcpy(&receivedData, incomingData, sizeof(receivedData));
  
//   // シリアルモニタに表示
//   Serial.print("Temperature: "); Serial.print(receivedData.temp); Serial.println(" C");
//   Serial.print("Humidity: "); Serial.print(receivedData.hum); Serial.println(" %");
//   Serial.print("Light: "); Serial.println(receivedData.light);
//   Serial.print("Distance: "); Serial.print(receivedData.distance); Serial.println(" cm");
//   Serial.println("---");
  
//   // LCD表示
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.print("T:");
//   lcd.print(receivedData.temp, 1);
//   lcd.print("C L:");
//   lcd.print(receivedData.light);
  
//   lcd.setCursor(0, 1);
//   lcd.print("H:");
//   lcd.print(receivedData.hum, 0);
//   lcd.print("% D:");
//   lcd.print(receivedData.distance, 0);
//   lcd.print("cm");
  
//   // 条件判定
//   isDark = (receivedData.light < DARK_THRESHOLD);
//   isAway = (receivedData.distance > AWAY_THRESHOLD);
  
//   // 暗くて人がいない場合に音を鳴らす
//   if (isDark && isAway && !alreadyPlayed) {
//     Serial.println(">>> ALERT: Dark and nobody detected! Playing sound...");
//     myDFPlayer.play(1);  // 0001_Pomodoro.mp3 を再生
//     alreadyPlayed = true;
//   } else if (!isDark || !isAway) {
//     // 条件が解除されたらフラグをリセット
//     alreadyPlayed = false;
//   }
// }

// void setup() {
//   Serial.begin(115200);
  
//   Serial.println("WAITING...");
//   delay(3000);
  
//   // DFPlayer初期化
//   mySoftwareSerial.begin(9600);
//   if (!myDFPlayer.begin(mySoftwareSerial)) {
//     Serial.println("ERROR: DFPlayer NOT FOUND");
//   } else {
//     Serial.println("SUCCESS: DFPlayer CONNECTED!");
//     myDFPlayer.volume(15);
//     delay(500);
//     myDFPlayer.play(1);  // 起動確認音（0001_Pomodoro.mp3）
//   }
  
//   // I2C初期化 (SDA=D2, SCL=D1)
//   Wire.begin(D2, D1);
  
//   // LCD初期化
//   lcd.init();
//   lcd.backlight();
//   lcd.setCursor(0, 0);
//   lcd.print("Node1 Ready");
  
//   // ESP-NOW初期化
//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();
  
//   if (esp_now_init() != 0) {
//     Serial.println("ESP-NOW Init Error");
//     return;
//   }
  
//   esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
//   esp_now_register_recv_cb(OnDataRecv);  // 受信コールバック登録
  
//   Serial.println("Node 1 Ready - Waiting for data...");
// }

// void loop() {
//   // 受信はコールバックで処理されるため、loopは空でOK
//   delay(100);
// }

#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

SoftwareSerial mySoftwareSerial(D7, D8);
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  mySoftwareSerial.begin(9600);
  
  if (myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Connected");
    
    int fileCount = myDFPlayer.readFileCounts();
    Serial.print("Total files: ");
    Serial.println(fileCount);
    
    myDFPlayer.volume(25);
    delay(500);
    
    // 1番から7番まで順番に再生してみる
    for(int i = 1; i <= 7; i++) {
      Serial.print("Playing track ");
      Serial.println(i);
      myDFPlayer.play(i);
      delay(5000); // 5秒待つ
    }
  }
}

void loop() {}
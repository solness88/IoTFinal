// #include <ESP8266WiFi.h>
// #include <espnow.h>
// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>
// #include <SoftwareSerial.h>
// #include <DFRobotDFPlayerMini.h>

// LiquidCrystal_I2C lcd(0x27, 16, 2);
// SoftwareSerial mySoftwareSerial(D7, D8); // RX, TX
// DFRobotDFPlayerMini myDFPlayer;

// int targetMinutes = 0;
// bool isRunning = false;
// unsigned long previousMillis = 0;
// long remainingSeconds = 0;

// unsigned long lastAlertTime = 0;
// const unsigned long ALERT_INTERVAL = 3000;

// struct Message {
//   char key;
//   int command;
// };
// Message incomingData;

// struct SensorData {
//   float temp;
//   float hum;
//   int light;
//   float distance;
// };

// bool tempAlertPlayed = false;
// bool humAlertPlayed = false;
// bool lightAlertPlayed = false;
// bool userAway = false;
// // void onDataRecv(uint8_t * mac, uint8_t *data, uint8_t len) {
// //   memcpy(&incomingData, data, sizeof(incomingData));

// //   // 数字キー入力：停止中のみ受け付け
// //   if (incomingData.key >= '0' && incomingData.key <= '9' && !isRunning) {
// //     targetMinutes = targetMinutes * 10 + (incomingData.key - '0');
// //     if (targetMinutes > 99) targetMinutes = 99;
// //     remainingSeconds = targetMinutes * 60;
// //     updateDisplay();
// //   }

// //   // D2ボタン（青）：開始と一時停止のトグル（切り替え）
// //   if (incomingData.command == 2) {
// //     if (remainingSeconds > 0) {
// //       isRunning = !isRunning; // 実行状態を反転
// //     if (isRunning) {
// //       myDFPlayer.play(1); // 再開時に音を鳴らす
// //     }
// //     updateDisplay();
// //     }
// //   }

// //   // D0ボタン（赤）：リセット機能のみ
// //   else if (incomingData.command == 1) {
// //     isRunning = false;
// //     targetMinutes = 0;
// //     remainingSeconds = 0;
// //     myDFPlayer.stop(); // 音声を止める
// //     updateDisplay();
// //   }
// // }


// void updateDisplay() {
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   if (isRunning) {
//     lcd.print("** FOCUSING **");
//   } else if (remainingSeconds > 0) {
//     lcd.print("== PAUSED ==");
//   } else {
//     lcd.print("Set Time: ");
//     lcd.print(targetMinutes);
//     lcd.print("m");
//   }

//   lcd.setCursor(0, 1);
//   int m = remainingSeconds / 60;
//   int s = remainingSeconds % 60;
//   lcd.printf("Time: %02d:%02d", m, s);
// }

// void onDataRecv(uint8_t * mac, uint8_t *data, uint8_t len) {
//   // Node3からのメッセージ（サイズ判定）
//   if (len == sizeof(Message)) {
//     memcpy(&incomingData, data, sizeof(incomingData));
    
//     // 数字キー入力：停止中のみ受け付け
//     if (incomingData.key >= '0' && incomingData.key <= '9' && !isRunning) {
//       targetMinutes = targetMinutes * 10 + (incomingData.key - '0');
//       if (targetMinutes > 99) targetMinutes = 99;
//       remainingSeconds = targetMinutes * 60;
//       updateDisplay();
//     }
    
//     // D2ボタン（青）：開始と一時停止のトグル
//     if (incomingData.command == 2) {
//       if (remainingSeconds > 0) {
//         isRunning = !isRunning;
//         if (isRunning) {
//           myDFPlayer.play(1);
//         }
//         updateDisplay();
//       }
//     }
    
//     // D0ボタン（赤）：リセット
//     else if (incomingData.command == 1) {
//       isRunning = false;
//       targetMinutes = 0;
//       remainingSeconds = 0;
//       myDFPlayer.stop();
//       updateDisplay();
//     }
//   }
  
//   // Node2からのセンサーデータ
//   else if (len == sizeof(SensorData)) {
//     unsigned long currentTime = millis();
//     if (currentTime - lastAlertTime < 2000) {  // 2秒以内ならスキップ
//       return;
//     }
//     lastAlertTime = currentTime;
    
//     SensorData sensorData;
//     memcpy(&sensorData, data, sizeof(sensorData));
    
//     // 1. 温度チェック (25度以上)
//     if (sensorData.temp >= 25.0 && !tempAlertPlayed) {
//       myDFPlayer.play(3);
//       tempAlertPlayed = true;
//     } else if (sensorData.temp < 25.0) {
//       tempAlertPlayed = false;
//     }
    
//     // 2. 湿度チェック (40%以下)
//     if (sensorData.hum <= 40.0 && !humAlertPlayed) {
//       myDFPlayer.play(4);
//       humAlertPlayed = true;
//     } else if (sensorData.hum > 40.0) {
//       humAlertPlayed = false;
//     }
    
//     // 3. 明るさチェック (200以下)
//     if (sensorData.light <= 200 && !lightAlertPlayed) {
//       myDFPlayer.play(5);
//       lightAlertPlayed = true;
//     } else if (sensorData.light > 200) {
//       lightAlertPlayed = false;
//     }
    
//     // 1. 離席判定（距離が一定以上離れたら：例 80cm以上）
//     if (sensorData.distance > 80.0 && !userAway) {
//       myDFPlayer.play(6); // Away detected.
//       userAway = true;
//       lastPlayTime = millis();
//     }
//     // 2. 戻り判定（距離が近くなったら：例 30cm以内）
//     else if (sensorData.distance < 30.0 && userAway) {
//       myDFPlayer.play(7); // Welcome back!
//       userAway = false;
//       lastPlayTime = millis();
//     }
//   }
// }

// void setup() {
//   Serial.begin(115200);
//   mySoftwareSerial.begin(9600);
//   lcd.init();
//   lcd.backlight();
//   lcd.print("System Ready");

//   if (!myDFPlayer.begin(mySoftwareSerial)) {
//     Serial.println("DFPlayer Error");
//   }
//   myDFPlayer.volume(20);

//   WiFi.mode(WIFI_STA);
//   if (esp_now_init() != 0) return;
//   esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
//   esp_now_register_recv_cb(onDataRecv);
// }

// void loop() {
//   if (isRunning && remainingSeconds > 0) {
//     unsigned long currentMillis = millis();
//     if (currentMillis - previousMillis >= 1000) {
//       previousMillis = currentMillis;
//       remainingSeconds--;
//       updateDisplay();

//       if (remainingSeconds <= 0) {
//         isRunning = false;
//         myDFPlayer.play(2); // 終了音（1000Hz以下のスピーカー）
//         lcd.clear();
//         lcd.print("Time Up!");
//       }
//     }
//   }
// }

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

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (isRunning) {
    lcd.print("** FOCUSING **");
  } else if (remainingSeconds > 0) {
    lcd.print("== PAUSED ==");
  } else {
    lcd.print("Set Time: ");
    lcd.print(targetMinutes);
    lcd.print("m");
  }

  lcd.setCursor(0, 1);
  int m = remainingSeconds / 60;
  int s = remainingSeconds % 60;
  lcd.printf("Time: %02d:%02d", m, s);
}

void onDataRecv(uint8_t * mac, uint8_t *data, uint8_t len) {
  // Node3からのメッセージ（サイズ判定）
  if (len == sizeof(Message)) {
    memcpy(&incomingData, data, sizeof(incomingData));
    
    // 数字キー入力：停止中のみ受け付け
    if (incomingData.key >= '0' && incomingData.key <= '9' && !isRunning) {
      targetMinutes = targetMinutes * 10 + (incomingData.key - '0');
      if (targetMinutes > 99) targetMinutes = 99;
      remainingSeconds = targetMinutes * 60;
      updateDisplay();
    }
    
    // D2ボタン（青）：開始と一時停止のトグル
    if (incomingData.command == 2) {
      if (remainingSeconds > 0) {
        isRunning = !isRunning;
        if (isRunning) {
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
  lcd.print("System Ready");

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Error");
    lcd.clear();
    lcd.print("DFPlayer Error");
    while(1);
  }
  Serial.println("DFPlayer OK");
  delay(500);
  myDFPlayer.volume(20);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) return;
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);
}

// void loop() {
//   // 離席検知処理
//   if (lastDistance >= 50.0 && !userAway) {
//     myDFPlayer.play(6);
//     userAway = true;
//     if (isRunning) {
//       isRunning = false;
//       updateDisplay();
//     }
//   } else if (lastDistance <= 30.0 && userAway) {
//     myDFPlayer.play(7);
//     userAway = false;
//     if (remainingSeconds > 0) {
//       isRunning = true;
//       updateDisplay();
//     }
//   }
  
//   // 既存のタイマー処理
//   if (isRunning && remainingSeconds > 0) {
//     unsigned long currentMillis = millis();
//     if (currentMillis - previousMillis >= 1000) {
//       previousMillis = currentMillis;
//       remainingSeconds--;
//       updateDisplay();

//       if (remainingSeconds <= 0) {
//         isRunning = false;
//         myDFPlayer.play(2);
//         lcd.clear();
//         lcd.print("Time Up!");
//       }
//     }
//   }
// }

void loop() {
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

  // 3. データを送信
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  // 4. 確認用にシリアルモニターに出力
  Serial.print("Temp: "); Serial.print(myData.temp);
  Serial.print(" Hum: "); Serial.print(myData.hum);
  Serial.print(" Light: "); Serial.print(myData.light);
  Serial.print(" Dist: "); Serial.println(myData.distance);

  delay(5000);
}
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// D1(RX), D2(TX) を使用。抵抗はD1側に！
SoftwareSerial mySoftwareSerial(D2, D1); 
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  
  Serial.println("Initializing DFPlayer...");

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("Unable to begin. Check SD card or wiring.");
    while(true);
  }
  
  Serial.println("DFPlayer Online!");
  
  // 音量を設定 (0~30)
  myDFPlayer.volume(20);  
  
  // 0001.mp3 を再生
  myDFPlayer.play(1); 
}

void loop() {
  // テストなのでループは空
}
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Keypad.h>

// --- 受信機（Node 1）のMACアドレスに書き換えてください ---
uint8_t receiverAddress[] = {0xC4, 0x5B, 0xBE, 0xF4, 0x2C, 0x67};

// --- キーパッドの設定 ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
{'1','2','3','A'},
{'4','5','6','B'},
{'7','8','9','C'},
{'*','0','#','D'}
};

// 配線: [左から1,2,3,4番] -> [D3, D4, D5, D6]
//      [左から5,6,7,8番] -> [D7, D8, 3, 1]
byte rowPins[ROWS] = {D3, D4, D5, D6};
byte colPins[COLS] = {D7, D8, 10, 9};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int btnStart = D1;
const int btnReset = D2;

struct Message {
char key;
int command;
};
Message myData;

void setup() {
Serial.begin(115200);

pinMode(btnStart, INPUT_PULLUP);
pinMode(btnReset, INPUT_PULLUP);

WiFi.mode(WIFI_STA);
WiFi.disconnect();

if (esp_now_init() != 0) {
Serial.println("ESP-NOW Init Failed");
return;
}

esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
esp_now_add_peer(receiverAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

Serial.println("Node 3 Ready.");
}

void loop() {
myData.key = '\0';
myData.command = 0;
bool shouldSend = false;

char customKey = keypad.getKey();
if (customKey) {
myData.key = customKey;
shouldSend = true;
Serial.print("Key: ");
Serial.println(myData.key);
}

if (digitalRead(btnStart) == LOW) {
myData.command = 2;
shouldSend = true;
Serial.println("Command: Start/Pause");
delay(300);
}

if (digitalRead(btnReset) == LOW) {
myData.command = 1;
shouldSend = true;
Serial.println("Command: Reset");
delay(300);
}

if (shouldSend) {
esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));
}
}


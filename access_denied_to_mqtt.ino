#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define RST_PIN  22  
#define SS_PIN   5   

#define EEPROM_SIZE 512       
#define ADDR_NUM_CARDS 0      
#define ADDR_UID_START 1      
#define MAX_CARDS 100         

// Wi-Fi နှင့် MQTT Settings (သင့်အိမ်သုံးအချက်အလက်ဖြင့် ပြင်ပါ)
const char* ssid = "Xiaomi 13";          
const char* password = "11228899";
const char* mqtt_server = "broker.hivemq.com"; 

WiFiClient espClient;
PubSubClient client(espClient);
MFRC522 mfrc522(SS_PIN, RST_PIN);

int systemMode = 0; 

void setup() {
  Serial.begin(115200);
  SPI.begin();      
  mfrc522.PCD_Init(); 

  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println(F("EEPROM စတင်၍ မရပါ!"));
    delay(1000);
  }

  // Smart Check (EEPROM တန်ဖိုး မှန်ကန်မှုရှိမရှိ စစ်ဆေးခြင်း)
  byte currentCards = EEPROM.read(ADDR_NUM_CARDS);
  if (currentCards > MAX_CARDS || currentCards == 0xFF) {
    EEPROM.write(ADDR_NUM_CARDS, 0);
    EEPROM.commit();
  }

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  printMenu();
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  if (Serial.available() > 0) {
    char command = Serial.read();
    
    if (command == '0') {
      systemMode = 0;
      Serial.println(F("\n--- Mode: Normal Access Control ---"));
    } 
    else if (command == '1') {
      systemMode = 1;
      Serial.println(F("\n--- Mode: ENROLL (ကတ်အသစ်ထည့်ရန် RFID ကပ်ပါ) ---"));
    } 
    else if (command == '2') {
      systemMode = 2;
      Serial.println(F("\n--- Mode: DELETE CARD BY INDEX ---"));
      listAllCards(); 
      Serial.println(F("ဖျက်လိုသော Card Index နံပါတ်ကို ရိုက်ထည့်ပါ:"));
      
      while (Serial.available() == 0) {}
      int indexToDelete = Serial.parseInt(); 
      deleteCardByIndex(indexToDelete);     
      
      systemMode = 0; 
      printMenu();
    }
  }

  if (systemMode == 0 || systemMode == 1) {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    String scannedUID = getUIDString(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.print(F("ဖတ်မိသော UID: "));
    Serial.println(scannedUID);

    if (systemMode == 0) {
      if (findCardInEEPROM() != -1) {
        Serial.println(F(">> ACCESS GRANTED! (ဝင်ခွင့်ပြုသည်)"));
        client.publish("esp32/rfid/access", "ACCESS_GRANTED");
      } else {
        Serial.println(F(">> ACCESS DENIED! (ဝင်ခွင့်ပိတ်ပင်သည်)"));
        client.publish("esp32/rfid/access", "ACCESS_DENIED");
      }
    } 
    else if (systemMode == 1) {
      addNewCardToEEPROM();
      systemMode = 0; 
      printMenu();
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(1000);
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println(F("\nWi-Fi ချိတ်ဆက်မှု အောင်မြင်ပါသည်။"));
  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print(F("MQTT ချိတ်ဆက်နေပါသည်..."));
    if (client.connect("ESP32_RFID_Client")) {
      Serial.println(F("ချိတ်ဆက်ပြီးပါပြီ။"));
    } else {
      delay(5000);
    }
  }
}

void printMenu() {
  Serial.println(F("\n========================================"));
  Serial.println(F(" COMMANDS (Serial တွင် ရိုက်ထည့်ပါ):"));
  Serial.println(F("  0 -> Normal Mode (Access Control)"));
  Serial.println(F("  1 -> Enroll Mode (ကတ်အသစ်ထည့်ရန်)"));
  Serial.println(F("  2 -> Delete Mode (ကတ်ကြည့်ပြီး ဖျက်ရန်)"));
  Serial.println(F("========================================"));
  Serial.print(F("စုစုပေါင်း သိမ်းထားသော ကတ်အရေအတွက်: "));
  Serial.println(EEPROM.read(ADDR_NUM_CARDS));
  Serial.println(F("----------------------------------------"));
}

void listAllCards() {
  byte numCards = EEPROM.read(ADDR_NUM_CARDS);
  Serial.println(F("--- Saved Cards List ---"));
  if (numCards == 0 || numCards > MAX_CARDS) {
    Serial.println(F("သိမ်းထားသော ကတ် မရှိသေးပါ။"));
    return;
  }
  for (int i = 0; i < numCards; i++) {
    int currentAddr = ADDR_UID_START + (i * 5);
    byte storedSize = EEPROM.read(currentAddr);
    Serial.print(i); 
    Serial.print(F(": "));
    for (byte j = 0; j < storedSize; j++) {
      byte b = EEPROM.read(currentAddr + 1 + j);
      if (b < 0x10) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  Serial.println(F("------------------------"));
}

int findCardInEEPROM() {
  byte numCards = EEPROM.read(ADDR_NUM_CARDS);
  if (numCards > MAX_CARDS) return -1;
  
  for (int i = 0; i < numCards; i++) {
    int currentAddr = ADDR_UID_START + (i * 5);
    byte storedSize = EEPROM.read(currentAddr);

    if (storedSize == mfrc522.uid.size) {
      bool match = true;
      for (byte j = 0; j < storedSize; j++) {
        if (EEPROM.read(currentAddr + 1 + j) != mfrc522.uid.uidByte[j]) {
          match = false;
          break;
        }
      }
      if (match) return i;
    }
  }
  return -1;
}

void addNewCardToEEPROM() {
  if (findCardInEEPROM() != -1) {
    Serial.println(F(">> Error: ဤကတ်သည် EEPROM ထဲတွင် ရှိပြီးသား ဖြစ်ပါသည်။"));
    return;
  }

  byte numCards = EEPROM.read(ADDR_NUM_CARDS);
  if (numCards >= MAX_CARDS) {
    Serial.println(F(">> Error: EEPROM ပြည့်သွားပါပြီ။"));
    return;
  }

  int writeAddr = ADDR_UID_START + (numCards * 5);
  EEPROM.write(writeAddr, mfrc522.uid.size);
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    EEPROM.write(writeAddr + 1 + i, mfrc522.uid.uidByte[i]);
  }
  
  EEPROM.write(ADDR_NUM_CARDS, numCards + 1);
  EEPROM.commit();
  Serial.println(F(">> SUCCESS: ကတ်အသစ် ထည့်သွင်းပြီးပါပြီ။"));
}

void deleteCardByIndex(int cardIndex) {
  byte numCards = EEPROM.read(ADDR_NUM_CARDS);
  
  if (cardIndex < 0 || cardIndex >= numCards) {
    Serial.println(F(">> Error: မှားယွင်းနေသော Card Index ဖြစ်ပါသည်။"));
    return;
  }

  for (int i = cardIndex; i < numCards - 1; i++) {
    int destAddr = ADDR_UID_START + (i * 5);
    int srcAddr = ADDR_UID_START + ((i + 1) * 5);
    
    for (int j = 0; j < 5; j++) {
      EEPROM.write(destAddr + j, EEPROM.read(srcAddr + j));
    }
  }

  EEPROM.write(ADDR_NUM_CARDS, numCards - 1);
  EEPROM.commit();
  Serial.println(F(">> SUCCESS: ကတ်ကို အောင်မြင်စွာ ဖျက်လိုက်ပါပြီ။"));
}

String getUIDString(byte *buffer, byte bufferSize) {
  String uidStr = "";
  for (byte i = 0; i < bufferSize; i++) {
    if (buffer[i] < 0x10) uidStr += "0";
    uidStr += String(buffer[i], HEX);
  }
  uidStr.toUpperCase();
  return uidStr;
}
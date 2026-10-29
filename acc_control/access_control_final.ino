#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <PubSubClient.h>

// 1. Hardware Pins သတ်မှတ်ခြင်း
#define RST_PIN  22  
#define SS_PIN   5   

// 2. EEPROM Memory သတ်မှတ်ချက်များ
#define EEPROM_SIZE 512       
#define ADDR_NUM_CARDS 0      
#define ADDR_UID_START 1      
#define MAX_CARDS 100         

// 3. Wi-Fi နှင့် MQTT Configuration 
const char* ssid = "Xiaomi 13";          
const char* password = "11228899";  
const char* mqtt_server = "broker.hivemq.com"; 

WiFiClient espClient;
PubSubClient client(espClient);
MFRC522 mfrc522(SS_PIN, RST_PIN);

int systemMode = 0; 

// 4. Offline Queue အတွက် Structure နှင့် နေရာသတ်မှတ်ခြင်း (အများဆုံး Event ၁၀ ခု သိမ်းမည်)
struct LogEvent {
  String uid;
  String result;
};
LogEvent offlineQueue[10];
int queueCount = 0;

void setup() {
  Serial.begin(115200);
  SPI.begin();      
  mfrc522.PCD_Init(); 

  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println(F("EEPROM စတင်၍ မရပါ!"));
    delay(1000);
  }

  // EEPROM Smart Check
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
  // MQTT ချိတ်ဆက်မှု ပြတ်တောက်နေပါက ပြန်လည်ချိတ်ဆက်ရန် ကြိုးစားနေမည် (Offline ဖြစ်နေလည်း Code မရပ်ပါ)
  if (!client.connected()) {
    reconnect_mqtt();
  } else {
    client.loop();
    // အင်တာနက်ပြန်လာပြီဆိုပါက Offline Queue ထဲက ဒေတာဟောင်းများကို Sync လုပ်မည်
    syncOfflineQueue();
  }

  // 1. Serial Monitor မှ Command ဝင်လာခြင်း ရှိမရှိ စစ်ဆေးခြင်း
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

  // 2. Normal Mode သို့မဟုတ် Enroll Mode ဖြစ်မှသာ RFID ကတ်ကို ဖတ်မည်
  if (systemMode == 0 || systemMode == 1) {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    String scannedUID = getUIDString(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.print(F("ဖတ်မိသော UID: "));
    Serial.println(scannedUID);

    if (systemMode == 0) {
      String accessResult = "";
      if (findCardInEEPROM() != -1) {
        Serial.println(F(">> ACCESS GRANTED! (ဝင်ခွင့်ပြုသည် - တံခါးပွင့်မည်)"));
        accessResult = "ACCESS_GRANTED";
      } else {
        Serial.println(F(">> ACCESS DENIED! (ဝင်ခွင့်ပိတ်ပင်သည်)"));
        accessResult = "ACCESS_DENIED";
      }

      // MQTT ချိတ်ဆက်ထားပါက ချက်ချင်းပို့မည်၊ မချိတ်ထားပါက Queue ထဲသို့ သိမ်းမည် (Offline Mode)
      String payload = "{\"uid\":\"" + scannedUID + "\",\"status\":\"" + accessResult + "\"}";
      
      if (client.connected()) {
        client.publish("esp32/rfid/access", payload.c_str());
        Serial.println(F(">> MQTT သို့ တိုက်ရိုက် ပို့ပြီးပါပြီ။"));
      } else {
        storeOfflineEvent(scannedUID, accessResult);
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

// ---------------- Wi-Fi ချိတ်ဆက်သည့် Function ----------------
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // အင်တာနက်ချိတ်နေစဉ်မှာတောင် စနစ်က တန်းလန်းမရပ်သွားအောင် Timeout သို့မဟုတ် Non-blocking သုံးနိုင်သည်
  int connAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && connAttempts < 20) {
    delay(500);
    Serial.print(F("."));
    connAttempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\nWi-Fi ချိတ်ဆက်မှု အောင်မြင်ပါသည်။"));
    Serial.print(F("IP Address: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("\nWi-Fi ချိတ်ဆက်၍ မရသေးပါ။ Offline မုဒ်ဖြင့် ဆက်လက်လည်ပတ်နေပါသည်။"));
  }
}

// ---------------- MQTT Reconnection (Background တွင် ချိတ်ရန်) ----------------
void reconnect_mqtt() {
  // မူလကလို while loop ကြီးနဲ့ ရပ်မနေစေဘဲ အကြိမ်ရေအလိုက် စစ်ဆေးခြင်းဖြင့် System ကို Smooth ဖြစ်စေသည်
  static unsigned long lastReconnectAttempt = 0;
  unsigned long now = millis();
  
  if (now - lastReconnectAttempt > 5000) { // ၅ စက္ကန့်တစ်ကြိမ် ချိတ်ရန် ကြိုးစားမည်
    lastReconnectAttempt = now;
    if (client.connect("ESP32_RFID_Client")) {
      Serial.println(F("MQTT သို့ ပြန်လည်ချိတ်ဆက်ပြီးပါပြီ။"));
    }
  }
}

// ---------------- Offline Queue ထဲသို့ သိမ်းဆည်းသည့် Function ----------------
void storeOfflineEvent(String uid, String result) {
  if (queueCount < 10) {
    offlineQueue[queueCount].uid = uid;
    offlineQueue[queueCount].result = result;
    queueCount++;
    Serial.print(F(">> Internet ပြတ်နေသဖြင့် Event ကို Offline Queue သို့ သိမ်းလိုက်ပါပြီ။ (စုစုပေါင်း: "));
    Serial.print(queueCount);
    Serial.println(F(" ခု)"));
  } else {
    Serial.println(F(">> Warning: Offline Queue ပြည့်နေပါပြီ။ Event အချို့ လွတ်သွားနိုင်ပါသည်။"));
  }
}

// ---------------- Wi-Fi ပြန်လာချိန်တွင် Queue ကို Sync လုပ်သည့် Function ----------------
void syncOfflineQueue() {
  if (queueCount > 0 && client.connected()) {
    Serial.println(F(">> အင်တာနက်လိုင်း ပြန်လာပါပြီ။ Offline Queue မှ ဒေတာများကို Sync လုပ်နေပါသည်..."));
    
    for (int i = 0; i < queueCount; i++) {
      String payload = "{\"uid\":\"" + offlineQueue[i].uid + "\",\"status\":\"" + offlineQueue[i].result + "\"}";
      client.publish("esp32/rfid/access", payload.c_str());
      delay(100); // Server ကို ဝန်မပိစေရန် ခေတ္တစောင့်မည်
    }
    
    queueCount = 0; // Sync လုပ်ပြီးပါက Queue ကို ရှင်းလင်းမည်
    Serial.println(F(">> Sync လုပ်ခြင်း အောင်မြင်ပြီးစီးပါပြီ။"));
  }
}

// ---------------- Menu ပြသသည့် Function ----------------
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

  int writeAddr = ADDR_NUM_CARDS + 1 + (numCards * 5); // Correction for address alignment
  EEPROM.write(ADDR_NUM_CARDS + 1 + (numCards * 5), mfrc522.uid.size);
  int baseAddr = ADDR_UID_START + (numCards * 5);
  EEPROM.write(baseAddr, mfrc522.uid.size);
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    EEPROM.write(baseAddr + 1 + i, mfrc522.uid.uidByte[i]);
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
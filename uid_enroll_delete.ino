#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

#define RST_PIN  22
#define SS_PIN   5

#define EEPROM_SIZE 64
#define ADDR_NUM_CARDS 0
#define ADDR_UID_START 1

MFRC522 mfrc522(SS_PIN, RST_PIN);

// Mode များကို သတ်မှတ်ခြင်း
// 0 = Normal Mode (Access Control)
// 1 = Enroll Mode (ကတ်အသစ်ထည့်ရန်)
// 2 = Delete Mode by Index (စာရင်းကြည့်၍ ဖျက်ရန်)
int systemMode = 0; 

void setup() {
  Serial.begin(115200);
  //while (!Serial);
  
  SPI.begin();      
  mfrc522.PCD_Init(); 

  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println(F("Failed to initialise EEPROM!"));
    delay(1000);
  }

  printMenu();
}

void loop() {
  // Serial Monitor မှ Command ပေးပို့ခြင်း ရှိမရှိ စစ်ဆေးခြင်း
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
      
      // သိမ်းထားသော ကတ်စာရင်းအားလုံးကို ထုတ်ပြခြင်း
      listAllCards(); 
      
      Serial.println(F("ဖျက်လိုသော Card Index နံပါတ်ကို ရိုက်ထည့်ပါ (ဥပမာ: 0, 1, 2...):"));
      
      // Serial မှ Input ရလာသည်အထိ စောင့်ဆိုင်းခြင်း
      while (Serial.available() == 0) {
        // User က ဂဏန်းရိုက်ထည့်ရန် စောင့်နေမည်
      }
      
      int indexToDelete = Serial.parseInt(); // ဝင်လာသောဂဏန်းကို ဖတ်ခြင်း
      deleteCardByIndex(indexToDelete);     // အဆိုပါ Index ဖြင့် ဖျက်ခြင်း
      
      systemMode = 0; // လုပ်ဆောင်ပြီးပါက Normal Mode သို့ ပြန်သွားမည်
      printMenu();
    }
  }

  // Normal Mode သို့မဟုတ် Enroll Mode အတွက်သာ RFID ကတ်ကို စစ်ဆေးမည်
  if (systemMode == 0 || systemMode == 1) {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    Serial.print(F("Scanned UID: "));
    printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();

    if (systemMode == 0) {
      // Normal Mode: ဝင်ခွင့်စစ်ဆေးခြင်း
      if (findCardInEEPROM() != -1) {
        Serial.println(F(">> ACCESS GRANTED! (ခွင့်ပြုထားသော ကတ်ဖြစ်ပါသည်)"));
      } else {
        Serial.println(F(">> ACCESS DENIED! (ခွင့်ပြုချက် မရှိပါ)"));
      }
    } 
    else if (systemMode == 1) {
      // Enroll Mode: ကတ်အသစ်ထည့်ခြင်း
      addNewCardToEEPROM();
      systemMode = 0; 
      printMenu();
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(1000);
  }
}

// ---------------- Menu ပြသရန် Function ----------------
void printMenu() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" SELECT MODE (Serial Monitor တွင် ရိုက်ထည့်ပါ):"));
  Serial.println(F("  0 -> Normal Mode (Access Control)"));
  Serial.println(F("  1 -> Add New Card (ကတ်အသစ်ထည့်ရန်)"));
  Serial.println(F("  2 -> Delete Card by Index (ကတ်စာရင်းကြည့်ပြီး ဖျက်ရန်)"));
  Serial.println(F("========================================"));
  Serial.print(F("Total saved cards: "));
  Serial.println(EEPROM.read(ADDR_NUM_CARDS));
  Serial.println(F("----------------------------------------"));
}

// ---------------- သိမ်းထားသော ကတ်များအားလုံးကို ထုတ်ပြသည့် Function ----------------
void listAllCards() {
  byte numCards = EEPROM.read(ADDR_NUM_CARDS);
  Serial.println(F("--- Saved Cards List ---"));
  if (numCards == 0) {
    Serial.println(F("No cards saved yet."));
    return;
  }
  
  for (int i = 0; i < numCards; i++) {
    int currentAddr = ADDR_UID_START + (i * 5);
    byte storedSize = EEPROM.read(currentAddr);
    
    Serial.print(i); // Index နံပါတ်
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

// ---------------- EEPROM ထဲတွင် ရှာရန် ----------------
int findCardInEEPROM() {
  byte numCards = EEPROM.read(ADDR_NUM_CARDS);
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

// ---------------- ကတ်အသစ်ထည့်ရန် ----------------
void addNewCardToEEPROM() {
  if (findCardInEEPROM() != -1) {
    Serial.println(F(">> Error: This card is already registered!"));
    return;
  }

  byte numCards = EEPROM.read(ADDR_NUM_CARDS);
  if (numCards >= 10) {
    Serial.println(F(">> Error: EEPROM is full!"));
    return;
  }

  int writeAddr = ADDR_UID_START + (numCards * 5);
  EEPROM.write(writeAddr, mfrc522.uid.size);
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    EEPROM.write(writeAddr + 1 + i, mfrc522.uid.uidByte[i]);
  }
  EEPROM.write(ADDR_NUM_CARDS, numCards + 1);
  EEPROM.commit();
  Serial.println(F(">> SUCCESS: New card added to EEPROM!"));
}

// ---------------- Index ဖြင့် ကတ်ဖျက်ရန် ----------------
void deleteCardByIndex(int cardIndex) {
  byte numCards = EEPROM.read(ADDR_NUM_CARDS);
  
  if (cardIndex < 0 || cardIndex >= numCards) {
    Serial.println(F(">> Error: Invalid card index!"));
    return;
  }

  // ဖျက်မည့်ကတ်နောက်တွင် ရှိသော ကတ်များကို ရှေ့သို့ တိုးခြင်း (Shift Data)
  for (int i = cardIndex; i < numCards - 1; i++) {
    int destAddr = ADDR_UID_START + (i * 5);
    int srcAddr = ADDR_UID_START + ((i + 1) * 5);
    
    for (int j = 0; j < 5; j++) {
      EEPROM.write(destAddr + j, EEPROM.read(srcAddr + j));
    }
  }

  // စုစုပေါင်း ကတ်အရေအတွက်ကို ၁ လျှော့ချခြင်း
  EEPROM.write(ADDR_NUM_CARDS, numCards - 1);
  EEPROM.commit();
  Serial.println(F(">> SUCCESS: Card deleted successfully by index!"));
}

// ---------------- Hex Display Helper ----------------
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    if (buffer[i] < 0x10) Serial.print(F(" 0"));
    else Serial.print(F(" "));
    Serial.print(buffer[i], HEX);
  }
}

#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

#define RST_PIN  22
#define SS_PIN   5

#define EEPROM_SIZE 64

MFRC522 mfrc522(SS_PIN, RST_PIN);

// -----------------------------------------------------------------
// ခွင့်ပြုလိုသော (Valid) Master UID ကို ဤနေရာတွင် သတ်မှတ်ပါ (ဥပမာ - 4 bytes)
// သင့်ကတ်၏ Actual UID ဖြင့် ပြောင်းလဲပေးပါ
byte validUID[] = {0x53, 0x97, 0x71, 0x14}; 
byte validUIDSize = sizeof(validUID);
// -----------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  SPI.begin();      
  mfrc522.PCD_Init(); 

  // EEPROM စတင်ခြင်း
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println(F("Failed to initialise EEPROM!"));
    delay(1000);
  }

  // ပထမဆုံးအကြိမ် (သို့မဟုတ် Valid UID ကို Update လုပ်လိုပါက) 
  // EEPROM ထဲသို့ Valid UID ကို အရင်ရေးသားသိမ်းဆည်းခြင်း
  saveValidUIDToEEPROM();

  Serial.println(F("=== RFID Access Control System Started ==="));
  Serial.println(F("Scan your RFID card..."));
  Serial.println();
}

void loop() {
  // ကတ်အသစ် ရှိမရှိ စစ်ဆေးခြင်း
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // ကတ်ရဲ့ UID ကို ဖတ်လို့ရမရ စစ်ဆေးခြင်း
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // ဖတ်ရလာသော ကတ်၏ UID ကို ပြသရန်
  Serial.print(F("Scanned UID: "));
  printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();

  // EEPROM ထဲတွင် သိမ်းထားသော Valid UID နှင့် တိုက်စစ်စစ်ခြင်း
  if (checkCardWithEEPROM()) {
    Serial.println(F(">> ACCESS GRANTED! (မှန်ကန်ပါသည်)"));
  } else {
    Serial.println(F(">> ACCESS DENIED! (ခွင့်ပြုချက် မရှိပါ)"));
  }

  Serial.println(F("-----------------------------------"));

  // ကတ်ကို ဆက်တိုက်ဖတ်နေခြင်း မဖြစ်စေရန် ရပ်တန့်ခြင်း
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  delay(2000); // ၂ စက္ကန့် စောင့်မည်
}

// -----------------------------------------------------------------
// Valid UID ကို EEPROM ထဲသို့ ရေးသွင်းသည့် လုပ်ဆောင်ချက်
void saveValidUIDToEEPROM() {
  EEPROM.write(0, validUIDSize); // Size ကို Address 0 မှာ သိမ်းမည်
  for (byte i = 0; i < validUIDSize; i++) {
    EEPROM.write(1 + i, validUIDInfo(i)); // Address 1 မှစ၍ UID bytes များကို သိမ်းမည်
  }
  EEPROM.commit();
  Serial.println(F("[System] Valid UID saved to EEPROM successfully."));
}

// helper function for array access
byte validUIDInfo(byte index) {
  return validUID[index];
}

// -----------------------------------------------------------------
// ဖတ်လာသော ကတ်ကို EEPROM ထဲက Valid UID နှင့် တိုက်စစ်သည့် လုပ်ဆောင်ချက်
bool checkCardWithEEPROM() {
  byte storedSize = EEPROM.read(0);

  // ကတ်၏ Size အရွယ်အစား မတူပါက မှားယွင်းသည်ဟု ယူဆမည်
  if (mfrc522.uid.size != storedSize) {
    return false;
  }

  // Byte တစ်လုံးချင်းစီ တိုက်စစ်ခြင်း
  for (byte i = 0; i < storedSize; i++) {
    byte storedByte = EEPROM.read(1 + i);
    if (mfrc522.uid.uidByte[i] != storedByte) {
      return false; // တစ်လုံးချင်းစီထဲမှာ မတူတာတွေ့ရင် false ပြန်မည်
    }
  }
  
  return true; // အားလုံးတူညီမှ true ပြန်မည်
}

// -----------------------------------------------------------------
// Hex ပုံစံဖြင့် Serial တွင် ထုတ်ပြရန် Helper Function
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    if (buffer[i] < 0x10) {
      Serial.print(F(" 0"));
    } else {
      Serial.print(F(" "));
    }
    Serial.print(buffer[i], HEX);
  }
}
#include <SPI.h>
#include <MFRC522.h>

// Pin သတ်မှတ်ချက်များ
#define RST_PIN  22  // Configurable, see typical pin layout above
#define SS_PIN   5   // Configurable, see typical pin layout above

// MFRC522 instance ဖန်တီးခြင်း
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  // Serial Monitor ကို စတင်ခြင်း
  Serial.begin(115200);
  while (!Serial); // Serial monitor ပွင့်လာသည်အထိ စောင့်ရန်
  
  // SPI bus ကို စတင်ခြင်း
  SPI.begin();      
  
  // MFRC522 reader ကို စတင်ခြင်း
  mfrc522.PCD_Init(); 
  
  // MFRC522 ရဲ့ version ကို စစ်ဆေးရန် (အဆင်ပြေမပြေ သိနိုင်ရန်)
  // mfrc522.PCD_DumpVersionToSerial();
  
  Serial.println(F("=== RFID MFRC522 Reader Started ==="));
  Serial.println(F("Scan your RFID card or key fob..."));
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

  // Serial Monitor တွင် UID ကို ထုတ်ပြရန်
  Serial.print(F("Card UID: "));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    // တစ်လုံးချင်းစီကို Hex ပုံစံဖြင့် 0 ဖြည့်ပြီးပြရန်
    if (mfrc522.uid.uidByte[i] < 0x10) {
      Serial.print(F(" 0"));
    } else {
      Serial.print(F(" "));
    }
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // (Optional) Card Type ကိုပါ သိလိုပါက -
  // byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  // Serial.print(F("PICC Type: "));
  // Serial.println(mfrc522.PICC_GetTypeName(piccType));

  // ကတ်ကို ဆက်တိုက်ဖတ်နေခြင်း မဖြစ်စေရန် ရပ်တန့်ခဏပေးခြင်း
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  Serial.println();
  delay(1000); // ၁ စက္ကန့် စောင့်မည်
}
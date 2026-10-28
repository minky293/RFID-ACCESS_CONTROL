#include <Arduino.h>

#define HLW_RX_PIN 26  // Schematic အရ HLW8032 မှလာသော H_TX သည် ESP32 ၏ GPIO 26 နှင့် ချိတ်ဆက်ထားသည်

byte SerialTemps[24];  // HLW8032 မှလာသော Data Packet တစ်ခုလျှင် 24 Bytes ပါဝင်သဖြင့် သိမ်းဆည်းရန် Array သတ်မှတ်ခြင်း
byte dataIndex = 0;    // Array ထဲသို့ Data များကို အစဉ်လိုက် (Index အလိုက်) ထည့်သွင်းရန် နေရာညွှန်ပြကိန်း

uint32_t VolPar = 0;   // HLW8032 ၏ Voltage Parameter တန်ဖိုးကို သိမ်းဆည်းရန် 32-bit ကိန်းပြည့် variable
uint32_t VolData = 1;  // HLW8032 ၏ Voltage Register တန်ဖိုး (သုညဖြင့် စားမိခြင်းကို ကာကွယ်ရန် 1 ဟု စတင်သတ်မှတ်ထားသည်)
float VF = 1880.0;     // VolR1 / VolR2 hardware resistors များမှ တွက်ချက်ထားသည့် Voltage Factor တန်ဖိုး

void setup() {
  Serial.begin(115200);  // ကွန်ပျူတာ၏ Serial Monitor နှင့် ချိတ်ဆက်ရန် Baud rate 115200 ဖြင့် စတင်ခြင်း
  delay(1000);           // စနစ် တည်ငြိမ်သွားစေရန် ၁ စက္ကန့် စောင့်ဆိုင်းခြင်း

  // Serial1 ကို 4800 baud, 8E1 (8 Data bits, Even Parity, 1 Stop bit) စနစ်ဖြင့် HLW8032 ဖတ်ရန် သတ်မှတ်ခြင်း
  // Pin 26 (HLW_RX_PIN) မှ ဒေတာဝင်မည်ဖြစ်ပြီး၊ TX ကို အသုံးမပြုပါ (-1)
  Serial1.begin(4800, SERIAL_8E1, HLW_RX_PIN, -1);
}

void loop() {
  // Serial1 တွင် ဖတ်စရာ Data များ ရှိနေသမျှ ဆက်တိုက် လုပ်ဆောင်မည်
  while (Serial1.available() > 0) {
    uint8_t incomingByte = Serial1.read();  // ဝင်လာသော Byte တစ်ခုချင်းစီကို ဖတ်ယူခြင်း

    // HLW8032 Packet ၏ ပထမ Header Byte (0x5A) ဟုတ်မဟုတ် စတင်စစ်ဆေးခြင်း
    if (dataIndex == 0 && incomingByte != 0x5A) {
      continue; // မဟုတ်ပါက ကျော်သွားမည် (ဒေတာအစကို ရှာနေခြင်း)
    }
    
    // HLW8032 Packet ၏ ဒုတိယ Header Byte (0x5A) ဟုတ်မဟုတ် စစ်ဆေးခြင်း
    if (dataIndex == 1 && incomingByte != 0x5A) {
      dataIndex = 0; // မှားယွင်းနေပါက Index ကို အစမှ ပြန်စရန် သုညသို့ ပြန်ပို့မည်
      continue;
    }

    // စစ်ဆေးချက်များ မှန်ကန်ပါက Array ထဲသို့ ဝင်လာသော Byte ကို သိမ်းဆည်းမည်
    SerialTemps[dataIndex] = incomingByte;
    dataIndex++; // နောက်ထပ် Byte အတွက် Index ကို တစ်တိုးမည်

    // ပက်ကတ်အရှည် 24 Bytes အပြည့်ရောက်ရှိသည့်အခါ
    if (dataIndex >= 24) {
      dataIndex = 0; // နောက်လာမည့် Packet အသစ်အတွက် Index ကို ပြန်လည် ရှင်းလင်းမည်
      
      // ပက်ကတ် အမှန်ကန်ဆုံးဖြစ်ကြောင်း Checksum ဖြင့် စစ်ဆေးခြင်း
      if (verifyChecksum()) {
        processVoltageData();  // မှန်ကန်ပါက ဗို့အားတန်ဖိုးကို ဆက်လက် တွက်ချက်ထုတ်ယူမည်
      }
    }
  }
}

// ရလာသော ပက်ကတ်သည် အမှန်ကန်ဆုံး ဟုတ်မဟုတ် (Data Integrity) စစ်ဆေးပေးသော ဖန်ရှင်
bool verifyChecksum() {
  byte checkSum = 0;
  // Index 2 မှ 22 အထိရှိသော Bytes တန်ဖိုးများကို အားလုံးပေါင်းထည့်ခြင်း
  for (int i = 2; i <= 22; i++) {
    checkSum += SerialTemps[i];
  }
  // ပေါင်းထားသော တန်ဖိုးသည် ပက်ကတ်၏ နောက်ဆုံး Checksum Byte (Index 23) နှင့် ကိုက်ညီမှုရှိမရှိ စစ်ဆေးပြီး True/False ပြန်ပေးမည်
  return (checkSum == SerialTemps[23]);
}

// ဗို့အား (Voltage) တန်ဖိုးကို တွက်ချက်ထုတ်ပေးသော ဖန်ရှင်
void processVoltageData() {
  // SerialTemps ထဲမှ Byte 2, 3, 4 ကို ပေါင်းစပ်၍ Voltage Parameter တန်ဖိုးကို ဖော်ထုတ်ခြင်း
  VolPar = ((uint32_t)SerialTemps[2] << 16) | 
           ((uint32_t)SerialTemps[3] << 8)  | 
            SerialTemps[4];

  // Byte 20 ၏ 6 ခုမြောက် Bit (Voltage Data Flag) သည် 1 ဟု ဖြစ်မဖြစ် စစ်ဆေးခြင်း (ဒေတာအသစ် ရောက်ရှိနေပြီလား သိရန်)
  if (bitRead(SerialTemps[20], 6) == 1) {
    // Byte 5, 6, 7 ကို ပေါင်းစပ်၍ လိုအပ်သော Voltage Register Data ကို ဖော်ထုတ်ခြင်း
    uint32_t tempVolData = ((uint32_t)SerialTemps[5] << 16) | 
                           ((uint32_t)SerialTemps[6] << 8)  | 
                            SerialTemps[7];

    if (tempVolData > 0) {  // သုညထက် ကြီးမှသာ ဆက်လက်တွက်ချက်မည် (Division by zero ကာကွယ်ရန်)
      VolData = tempVolData;

      // တကယ့် AC ဗို့အား (Voltage) တန်ဖိုးကို ဖော်မြူລາဖြင့် တွက်ချက်ခြင်း
      float voltage = ((float)VolPar / (float)VolData) * VF;

      // တွက်ချက်ရရှိလာသော ဗို့အားတန်ဖိုးကို ဒသမ ၂ ကိန်းအထိ Serial Monitor ပေါ်သို့ ထုတ်ပြခြင်း
      Serial.println(voltage, 2);
    }
  }
}
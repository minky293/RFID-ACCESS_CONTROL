# 🚀 ESP32 Smart Access Control System with Offline Support

An IoT-based RFID Access Control system built with an ESP32 microcontroller, MFRC522 RFID reader, EEPROM memory emulation, and MQTT protocol integration[cite: 11]. It features robust offline capabilities, allowing local access decisions and event queuing even when the internet connection is lost[cite: 11].

---

## 📌 Key Features

### A. RFID Management
* **Database & Whitelist:** Fast UID verification against stored authorization lists.
* **Access Control:** Triggers relay/buzzer for granted access and logs denied attempts locally.

### B. Offline & Storage Support
* **EEPROM Persistence:** Retains whitelist configurations and settings through power reboots.
* **Event Queuing:** Queues access logs locally when offline and syncs with the server once reconnected.

### C. MQTT Cloud Integration
* Real-time status reporting and event publishing over MQTT brokers.
* Non-blocking Wi-Fi and MQTT reconnection logic.

---

## 🛠️ Hardware Requirements
* **Microcontroller:** ESP32 Development Board
* **RFID Reader Module:** MFRC522 (SPI Interface)
* **Storage:** Internal EEPROM / LittleFS
* **Feedback:** Buzzer, LEDs, or Relay Module for door locks

---

## 💻 Getting Started
1. Open your project folder in **VS Code (PlatformIO)** or **Arduino IDE**.
2. Install the required libraries (`MFRC522`, `PubSubClient`).
3. Flash the `access_control_final.ino` code to your ESP32 board.
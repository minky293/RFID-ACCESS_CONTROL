# ESP32 Smart Access Control System with Offline Support

An IoT-based RFID Access Control system built with an ESP32 (WROOM-DA) microcontroller, MFRC522 RFID reader, EEPROM memory emulation, and MQTT protocol integration. It features robust offline capabilities, allowing local access decisions and event queuing even when the internet connection is lost.

---

## 🚀 Key Features

### A. RFID Management
* Reads and processes RFID card UIDs accurately.
* Differentiates between Authorized and Unauthorized cards.
* Supports enrolling (adding) new cards and deleting existing cards via Serial Monitor.
* Prevents duplicate card registrations.

### B. EEPROM Data Management
* Stores authorized card UIDs persistently in ESP32's flash memory via EEPROM emulation.
* Data persists across power cycles, reboots, and system resets (`EEPROM.commit()`).
* Includes smart checking for uninitialized memory (`0xFF` / garbage values) and capacity limits.

### C. Wi-Fi Reconnection Logic
* Monitors Wi-Fi connection status in real-time.
* Implements non-blocking / timeout-based reconnection logic so the system never freezes during connection drops.
* Automatically checks and restores MQTT connections once Wi-Fi is re-established.

### D. MQTT Reconnection Logic & Offline Queue
* Connects to standard MQTT brokers to publish access logs (`ACCESS_GRANTED` / `ACCESS_DENIED`).
* Background reconnection handler using `millis()` to prevent CPU blocking.
* **Offline Support:** Automatically stores access events in a RAM-based queue if the network is down, and syncs/publishes pending events back to the server once connection is restored.

### E. System Reliability
* **Local-First Architecture:** Door access control operates instantly via local EEPROM checks, regardless of Wi-Fi or MQTT availability.
* Robust error handling and logging via Serial Monitor.

---

## 📐 Data Structure & EEPROM Memory Map

* **Address `0` (1 Byte):** Total number of saved cards (`numCards`).
* **Address `1` onwards (5 Bytes per card):**
  * **Byte 0 (1 Byte):** Card UID size (`uid.size`).
  * **Bytes 1 to 4 (4 Bytes):** Card UID byte values (`uid.uidByte`).

---

## 🔌 Connection Diagram (Pinout)

| MFRC522 Module Pin | ESP32 (WROOM-DA) Pin | Description |
| :--- | :--- | :--- |
| **SDA (SS)** | GPIO 5 | Slave Select |
| **SCK** | GPIO 18 | SPI Clock |
| **MOSI** | GPIO 23 | SPI Master Out Slave In |
| **MISO** | GPIO 19 | SPI Master In Slave Out |
| **IRQ** | Not Connected | - |
| **GND** | GND | Ground |
| **RST** | GPIO 22 | Reset Pin |
| **3.3V** | 3.3V | Power Supply (**Strictly 3.3V only**) |

---

## 📦 Required Arduino Libraries

Make sure to install the following libraries through the Arduino IDE Library Manager:
* **MFRC522** (by GithubCommunity / Snipeye)
* **PubSubClient** (by Nick O'Leary)
* **EEPROM** (Built-in with ESP32 board package)
* **WiFi** (Built-in with ESP32 board package)

---

## 🧪 How to Use & Test

1. **Upload the Code:** Open the sketch in Arduino IDE, select your ESP32 board, update your Wi-Fi SSID, Password, and MQTT Broker details, and upload.
2. **Open Serial Monitor:** Set baud rate to `115200`.
3. **Menu Options:**
   * **`0`**: Normal Access Control Mode (Default).
   * **`1`**: Enroll Mode (Scan a new RFID card to save it).
   * **`2`**: Delete Mode (View saved card list and delete by index number).
4. **Offline Testing:** Disconnect your Wi-Fi router; notice that authorized cards still grant local access while events are safely queued. Reconnect Wi-Fi to watch the queue automatically sync to the MQTT broker.

---

## 📄 License
This project is open-source and available under the [MIT License](LICENSE).
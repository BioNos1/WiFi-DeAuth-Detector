# WiFi DeAuth Detector



## Overview

WiFi DeAuth Detector is an ESP8266-based wireless security monitoring device designed to detect abnormal Wi-Fi deauthentication activity in the surrounding environment.

A deauthentication attack is a type of wireless disruption attack where specially crafted management frames are sent to force devices to disconnect from a Wi-Fi network. This project demonstrates how an ESP8266 can be used as a lightweight security monitoring tool to identify suspicious wireless activity.

The device continuously monitors Wi-Fi traffic, analyzes wireless management frames, and provides real-time alerts when a possible deauthentication event is detected.

This project was developed for educational purposes to understand wireless security concepts, Wi-Fi management frames, and embedded system development.

---

# Features

- Real-time Wi-Fi deauthentication packet detection
- ESP8266 NodeMCU based design
- TFT display monitoring interface
- Buzzer alert system
- Packet counting system
- Configurable detection threshold
- Portable and low-cost hardware implementation
- Arduino IDE compatible

---

# Hardware Components

| Component | Description |
|-----------|-------------|
| ESP8266 NodeMCU | Main processing and Wi-Fi monitoring module |
| ST7735 TFT Display | Real-time status and packet information display |
| Buzzer | Audible security alert |
| Jumper Wires | Hardware connections |
| USB Power Source | Device power supply |

---

# Wiring Diagram

The hardware connections are shown in the following schematic:

![Wiring Diagram](WIRING.png)

## Pin Connections

| Component | ESP8266 Pin |
|-----------|-------------|
| TFT MOSI | D7 |
| TFT SCK | D5 |
| TFT CS | D8 |
| TFT DC | D4 |
| TFT RST | D3 |
| Buzzer Signal | D1 |
| Buzzer VCC | 3.3V |
| Ground | GND |

---

# Working Principle

The ESP8266 operates in monitor mode and observes wireless traffic in the surrounding area.

The device analyzes Wi-Fi management frames and identifies patterns associated with deauthentication activity. When the number of detected deauthentication packets exceeds the configured threshold, the system:

1. Counts suspicious packets.
2. Updates the TFT display.
3. Activates the buzzer alert.
4. Notifies the user about possible Wi-Fi disruption activity.

---

# Software Requirements

## Development Environment

- Arduino IDE

## Required Libraries

- ESP8266WiFi Library
- Adafruit GFX Library
- Adafruit ST7735 Library
- SPI Library

---

# Installation

1. Install Arduino IDE.
2. Install ESP8266 board support package.
3. Install the required libraries.
4. Download this repository.
5. Open:


> **Note**
>
> By default, the detector counts every **Deauthentication** and **Disassociation** frame it captures, regardless of which Wi-Fi network the packets belong to. In environments with multiple nearby access points, this can result in alerts triggered by activity unrelated to the network you intend to monitor.
> To improve accuracy, the detector can be configured to monitor a **specific Wi-Fi Access Point (AP)** by filtering packets using its **BSSID (MAC address)**. With this feature enabled, only management frames associated with the selected access point are counted, reducing false positives and making the detector more suitable for monitoring a single network.
## Target BSSID Filtering

This project supports monitoring a **specific Wi-Fi Access Point (AP)** by filtering captured IEEE 802.11 management frames using the access point's **BSSID (MAC address)**.

Instead of counting every deauthentication or disassociation frame detected nearby, the detector can be configured to monitor only a selected network. This reduces false positives and improves detection accuracy.

### Configure the Target BSSID

Replace the placeholder MAC address with your router's actual BSSID.

Example BSSID: 3C:84:6A:12:34:56 
const uint8_t TARGET_BSSID[6] = {
  0x3C, 0x84, 0x6A, 0x12, 0x34, 0x56
}; // this is example how to specify your mac address

```cpp
// ===== Target Network ===== //
// Replace with your router's MAC address (BSSID)
const uint8_t TARGET_BSSID[6] = {
  0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF  
};
```

### MAC Address Comparison Function

```cpp
bool compareMAC(const uint8_t *mac1, const uint8_t *mac2) {
  for (int i = 0; i < 6; i++) {
    if (mac1[i] != mac2[i]) return false;
  }
  return true;
}
```

### Updated Packet Sniffer

Replace your existing `sniffer()` function with the following implementation:

```cpp
void sniffer(uint8_t *buf, uint16_t len) {

  if (!buf || len < 28)
    return;

  // Frame Control field
  uint16_t frameControl = buf[0] | (buf[1] << 8);

  // Management subtype
  uint8_t subtype = (frameControl >> 4) & 0x0F;

  // Only Deauthentication (12) and Disassociation (10)
  if (subtype != 10 && subtype != 12)
    return;

  // IEEE 802.11 Management Frame
  const uint8_t *addr1 = &buf[4];   // Destination
  const uint8_t *addr2 = &buf[10];  // Source
  const uint8_t *addr3 = &buf[16];  // BSSID

  // Count only packets involving our target AP
  if (compareMAC(addr2, TARGET_BSSID) ||
      compareMAC(addr3, TARGET_BSSID)) {

    packet_rate++;

    Serial.print("Target packet detected: ");
    Serial.println(packet_rate);
  }
}
```

### How It Works

* The detector captures IEEE 802.11 management frames in promiscuous mode.
* Only **Deauthentication** and **Disassociation** frames are processed.
* The firmware compares the frame's **Source Address** and **BSSID** with the configured `TARGET_BSSID`.
* If either address matches the configured access point, the packet is counted.
* When the number of detected packets exceeds the configured threshold, the TFT display and buzzer alert the user.

> **Note:** Replace the placeholder BSSID with the MAC address of the access point you want to monitor.

Future Updates

This project will continue to evolve with new features and enhancements. Planned updates include:

Integration of Bluetooth (BLE) functionality for local wireless monitoring and notifications.
A web-based alarm and monitoring dashboard for real-time attack alerts, device status, and event logging.
Further optimization of the detection algorithm to improve accuracy and reduce false positives.
Additional customization options and enhanced user interface features.

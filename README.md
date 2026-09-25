# SENTINEL — Smart Emergency & Health Tracking System

> Real-time wearable IoT safety monitor for industrial workers. Low-cost, offline-ready, and engineered for rapid emergency response using bidirectional ESP-NOW communication.

[![License: Apache-2.0](https://img.shields.io/badge/License-Apache--2.0-blue.svg)](LICENSE)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](./)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)

---

## Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Project Gallery](#project-gallery)
- [System Architecture](#system-architecture)
- [How It Works](#how-it-works)
- [Technology Stack](#technology-stack)
- [Hardware BOM & Wiring](#hardware-bom--wiring)
- [Quick Start](#quick-start)
- [Communication Protocol](#communication-protocol)
- [Edge Safety Logic & Thresholds](#edge-safety-logic--thresholds)
- [Web Dashboard](#web-dashboard)
- [Customization Guide](#customization-guide)
- [Project Structure](#project-structure)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)
- [Team & Credits](#team--credits)

---

## Overview

**SENTINEL** is an industrial IoT safety and telemetry system designed to monitor worker vitals, environmental hazards, and physical trauma in real time. Operating independently of external cloud infrastructure or local internet connectivity, SENTINEL utilizes **ESP-NOW** for peer-to-peer radio communication between worker wearables and an on-site Admin Hub.

The worker node performs edge processing for fall detection and environmental hazard monitoring, while the Admin Hub aggregates multi-node telemetry, manages peer routing, and hosts a live WebSocket dashboard over a dedicated local Wi-Fi Access Point.

---

## Key Features

- **4-Stage Fall Detection State Machine:** Multi-phase acceleration tracking (`Free Fall → Impact → Watching Recovery → Confirmed`) eliminates false positives from everyday worker movement.
- **Full-Duplex ESP-NOW Protocol:** Dynamic peer discovery and bidirectional frame exchange on channel 1 with sub-10ms transmission latency.
- **Persistent "Sticky SOS":** Critical alerts latch into active state and require manual operator acknowledgment via the dashboard or physical reset on the device.
- **On-Device Edge Safety Logic:** Local classification into `SAFE`, `WARN`, `DANGER`, and `EMERGENCY` states; alarms sound immediately even during radio link dropouts.
- **Real-Time Industrial Dashboard:** Lightweight browser UI served from LittleFS featuring SVG sparklines, circular dose gauges, shift analytics, audio alarms, and manual ACK controls.
- **Adaptive Telemetry Cadence:** Automatic throttling between 2-second nominal heartbeats and 500ms high-priority emergency streams.

---

## Project Gallery

| Physical Wearable Prototype | Emergency Alert & Bidirectional Comms |
|:-------------------------:|:-----------------------------------:|
| ![Wearable Prototype](images/Top%20View.png) | ![Side View](images/Side%20View.png) |

**Live Dashboard & Analytics**

![Dashboard View](images/Emergency.png)

---

## System Architecture

```
                      ┌─────────────────────────────────┐
                      │    Worker Wearable Node(s)      │
                      │  - MPU6050, DHT22, MQ-2, MAX    │
                      │  - Edge Safety & Fall Detection │
                      │  - SSD1306 OLED, LEDs & Buzzer  │
                      └────────────────┬────────────────┘
                                       │
                      Telemetry Packet │ Hub Commands
                      (55 Bytes)       │ (20 Bytes)
                      [ESP-NOW 2.4GHz] │ [ESP-NOW 2.4GHz]
                                       │
                                       ▼
                      ┌─────────────────────────────────┐
                      │      SENTINEL Admin Hub         │
                      │  - ESP-NOW Peer Manager         │
                      │  - Wi-Fi SoftAP ("SENTINEL-HUB")│
                      │  - AsyncWebServer & WebSockets  │
                      │  - LittleFS Web Storage         │
                      └────────────────┬────────────────┘
                                       │
                          JSON Stream  │ Control Commands
                          over /ws     │ (manualAck, sendAlert)
                                       │
                                       ▼
                      ┌─────────────────────────────────┐
                      │     Web Operator Dashboard      │
                      │  - Live Gauges & Sparklines     │
                      │  - Shift History & Event Log    │
                      │  - Web Audio Emergency Alarm    │
                      │  - Emergency Acknowledgment     │
                      └─────────────────────────────────┘
```

---

## How It Works

1. **Local Sampling & Edge Evaluation:** The worker node samples accelerometer telemetry at 50 Hz (20ms), gas/vitals at 10 Hz (100ms), and ambient conditions every 2 seconds. The on-device safety engine computes local hazard states.
2. **Adaptive Wireless Broadcast:** Telemetry packets (`SentinelPacket`, 55 bytes) stream to the Hub via ESP-NOW at 2s intervals under normal conditions, accelerating to 500ms when status is `DANGER` or `EMERGENCY`.
3. **Hub Ingestion & Peer Tracking:** The Admin Hub automatically registers incoming MAC addresses as dynamic peers, tracks connection timeouts (10s threshold), and manages alarm states.
4. **WebSocket Fan-Out:** Telemetry is serialized into JSON and pushed across WebSocket clients every 2.5s or instantly upon event state changes.
5. **Bidirectional Acknowledgment & Dispatch:** Operators can trigger inline warnings or acknowledge critical emergencies from the UI. Commands (`HubCommand`, 20 bytes) route over ESP-NOW back to the specific worker MAC with checksum validation.

---

## Technology Stack

| Layer | Technologies & Libraries |
|:---|:---|
| **Hardware** | ESP32-WROOM-32, MPU6050 (6-DOF IMU), DHT22 (AM2302), MQ-2 Gas Sensor, MAX30102, SSD1306 0.96" OLED, Piezo Buzzer, Status LEDs |
| **Firmware** | C/C++ (Arduino Core / PlatformIO), `esp_now`, `esp_wifi`, `esp_task_wdt`, `ESPAsyncWebServer`, `AsyncTCP`, `ArduinoJson`, `LittleFS`, `Adafruit_MPU6050`, `Adafruit_SSD1306`, `DHT sensor library` |
| **Dashboard** | Vanilla JavaScript (ES6+), WebSockets API, Web Audio API, SVG Sparklines & Circular Gauges, CSS3 Variables & Flexbox/Grid |
| **Networking** | 2.4 GHz ESP-NOW (CSMA/CA, MAC-ACK), IEEE 802.11 b/g/n SoftAP, WebSocket RFC 6455 |

---

## Hardware BOM & Wiring

### Bill of Materials (Per Worker Node)

- 1x ESP32 Development Board (30-pin or 38-pin DevKit)
- 1x MPU6050 6-Axis Accelerometer & Gyroscope Module
- 1x DHT22 Temperature & Humidity Sensor
- 1x MQ-2 Hazardous Gas Sensor Module
- 1x MAX30102 Pulse Oximeter & Heart-Rate Sensor
- 1x SSD1306 128x64 I2C OLED Display
- 3x Push Buttons (Panic SOS, System Reset, Zone Select)
- 3x 3mm/5mm LEDs (Green, Yellow, Red) with 220Ω current-limiting resistors
- 1x 5V Active Piezo Buzzer
- 1x Prototype Breadboard / Perfboard & Jumper Wiring

### Node Pin Mapping

| Peripheral / Signal | ESP32 GPIO Pin | Mode / Notes |
|:---|:---|:---|
| **I2C SDA (OLED + MPU6050)** | `GPIO 21` | Hardware I2C Data (Wire) |
| **I2C SCL (OLED + MPU6050)** | `GPIO 22` | Hardware I2C Clock (Wire) |
| **DHT22 Data** | `GPIO 4` | Digital I/O (Single-bus) |
| **MQ-2 Digital Out (Gas)** | `GPIO 33` | Digital Input (Active LOW) |
| **Panic Button (SOS)** | `GPIO 16` | `INPUT_PULLUP` (Hold 1s to trigger) |
| **Reset Button** | `GPIO 5` | `INPUT_PULLUP` (Instant clear) |
| **Zone Switch Button** | `GPIO 27` | `INPUT_PULLUP` (Hold 3s to cycle) |
| **Green Status LED (Safe)** | `GPIO 2` | Digital Output |
| **Red Status LED (Emergency)** | `GPIO 12` | Digital Output |
| **Yellow Status LED (Warn)** | `GPIO 13` | Digital Output |
| **Buzzer** | `GPIO 14` | PWM / Tone Generator Output |

*Note: For the Admin Hub ESP32, `GPIO 2` is assigned to the onboard buzzer.*

---

## Quick Start

### 1. Firmware Configuration & Flashing

1. **Admin Hub:**
   - Source: [`admin_src/main.cpp`](admin_src/main.cpp) and [`admin_src/packet_defs.h`](admin_src/packet_defs.h).
   - Set `WORKER_COUNT` to the desired number of active nodes.
   - Flash to the Hub ESP32.

2. **Worker Node(s):**
   - Source: [`node_src/main.cpp`](node_src/main.cpp) and [`node_src/packet_defs.h`](node_src/packet_defs.h).
   - Set a unique `WORKER_ID` (e.g. `1`, `2`) in `node_src/main.cpp`.
   - Update `HUB_MAC` with the MAC address printed to serial by your Hub on boot.
   - Flash to each worker ESP32.

*See [CONTRIBUTING.md](CONTRIBUTING.md) for PlatformIO build configurations and library specifications.*

### 2. Dashboard Upload

Upload the web application files from [`docs/`](docs) (`index.html`, `style.css`, `app.js`) to the Hub's SPIFFS/LittleFS flash filesystem using the ESP32 LittleFS filesystem uploader.

### 3. Operation

1. Power on the Admin Hub. It launches the `SENTINEL-HUB` Wi-Fi AP (Default Password: `sentinel123`, Channel `1`).
2. Connect your browser device to `SENTINEL-HUB` and navigate to `http://192.168.4.1`.
3. Power on worker nodes. Nodes automatically register with the Hub and begin transmitting telemetry.

---

## Communication Protocol

SENTINEL uses binary packed structs across ESP-NOW to maintain zero protocol overhead and deterministic packet layouts. Both structures are declared with `#pragma pack(push, 1)` and verified at compile time with `static_assert`.

### 1. Telemetry Frame: `SentinelPacket` (55 Bytes)

Direction: **Worker Node → Admin Hub**

| Offset (Bytes) | Field Name | Data Type | Description |
|:---|:---|:---|:---|
| `0` | `workerID` | `uint8_t` | Unique worker node identifier (`1` to `N`) |
| `1` | `zoneID` | `uint8_t` | Current zone index (`1` to `6`) |
| `2 - 21` | `zoneName` | `char[20]` | Human-readable zone string (e.g. `"CNC Bay"`) |
| `22 - 25` | `totalAccel` | `float` (4B) | Vector magnitude in g: `sqrt(ax² + ay² + az²) / 9.81` |
| `26` | `fallDetected` | `bool` (1B) | Latch status for confirmed fall |
| `27` | `gasDetected` | `bool` (1B) | Active gas hazard flag |
| `28 - 31` | `tempC` | `float` (4B) | Compensated ambient temperature in °C |
| `32 - 35` | `humidity` | `float` (4B) | Relative humidity percentage |
| `36 - 39` | `soundDB` | `float` (4B) | Acoustic noise level in decibels |
| `40 - 43` | `noiseDosePct` | `float` (4B) | Shift noise dose percentage (`40dB–90dB` scale) |
| `44 - 47` | `heatDosePct` | `float` (4B) | Shift heat strain index (`20°C–40°C` scale) |
| `48` | `heartRate` | `uint8_t` | Heart rate in BPM |
| `49` | `spO2` | `uint8_t` | Blood oxygen saturation percentage |
| `50` | `safetyStatus` | `uint8_t` | Master safety state (`0`=SAFE, `1`=WARN, `2`=DANGER, `3`=EMERGENCY) |
| `51` | `emergencyType` | `uint8_t` | Specific hazard category code (`0` to `6`) |
| `52` | `panicPressed` | `bool` (1B) | Manual panic button triggered |
| `53` | `batteryPct` | `uint8_t` | Battery state of charge percentage |
| `54` | `fallStateCode` | `uint8_t` | Raw state machine status code (`0` to `4`) |

### 2. Command Frame: `HubCommand` (20 Bytes)

Direction: **Admin Hub → Worker Node**

| Offset (Bytes) | Field Name | Data Type | Description |
|:---|:---|:---|:---|
| `0` | `targetWorkerID` | `uint8_t` | Target node ID (`0` = Broadcast) |
| `1` | `messageType` | `uint8_t` | Command type (`0`=ACK, `1`=Gas, `2`=Noise, `3`=Heat, `4`=General) |
| `2` | `priority` | `uint8_t` | Execution priority (`1`=Info, `2`=Warn, `3`=Critical) |
| `3` | `checksum` | `uint8_t` | XOR error check over header and custom payload |
| `4 - 19` | `customMsg` | `char[16]` | Null-terminated alert text displayed on OLED |

### Checksum Calculation

```cpp
inline uint8_t computeChecksum(const HubCommand* cmd) {
  uint8_t cs = cmd->targetWorkerID ^ cmd->messageType ^ cmd->priority;
  for (int i = 0; i < 16; i++) {
    cs ^= cmd->customMsg[i];
  }
  return cs;
}
```

---

## Edge Safety Logic & Thresholds

### 4-Stage Fall Detection State Machine

To prevent false triggers from normal industrial motions (bending, jumping, fast walking), SENTINEL evaluates acceleration vectors sequentially:

```
 [ NO_FALL ]
      │  Total Accel < 0.3g (Free Fall)
      ▼
 [ FREE_FALL_DETECTED ]
      │  Total Accel > 3.0g within 500ms (Impact)
      ▼
 [ IMPACT_DETECTED ]
      │  Sustain for > 1000ms
      ▼
 [ WATCHING_RECOVERY ]
      │  No recovery movement (> 3000ms and Accel < 1.5g)
      ▼
 [ FALL_CONFIRMED ] ──► (Latches until BTN_RESET or RESCUE ACK)
```

### Safety Classification Matrix

The on-node safety engine executes `computeSafety()` every loop cycle:

| Level | Code | Classification | Trigger Conditions & Thresholds | Node Response |
|:---|:---|:---|:---|:---|
| **0** | `SAFE` | Nominal | All sensors within standard operating limits | Green LED ON, nominal OLED dashboard, 2s beacon |
| **1** | `WARN` | Warning | Temp: `40°C–45°C` \| Sound: `85–95 dB` \| HR: `120–150` or `45–55 BPM` \| SpO₂: `90%–94%` | Yellow LED ON, dashboard warning, 2s beacon |
| **2** | `DANGER` | High Hazard | Temp: `>45°C` \| Sound: `>95 dB` \| HR: `>150` or `<45 BPM` \| SpO₂: `<90%` | Red LED ON, OLED warning popup, 500ms beacon |
| **3** | `EMERGENCY` | Critical Alert | **Gas Detected** \| **Panic Pressed (Hold 1s)** \| **Fall Confirmed** | Red LED flashing, active buzzer alarm, OLED Emergency screen, 500ms beacon |

---

## Web Dashboard

The SENTINEL web interface is a single-page reactive dashboard with zero external dependencies:

- **Telemetry Sparklines:** Real-time polyline history charts for Temperature, Humidity, Noise, Heart Rate, and SpO₂.
- **Dose Gauges:** SVG circular gauges depicting cumulative Heat and Noise dose indices over shift duration.
- **Shift Distribution Timeline:** Visual 20-segment timeline tracking worker safety states.
- **Audio Alarm Engine:** Synthesizes pulsing square-wave acoustic warnings via the browser `AudioContext` during unacknowledged emergencies.
- **Event Logging & Analytics:** Records all state transitions and calculates shift statistics (Safe time %, incident count, peak exposures).
- **Built-in Mock/Demo Mode:** Opening `docs/index.html` on any standard web browser or local file path automatically engages synthetic data generator mode for testing UI animations and components without hardware.

---

## Customization Guide

Key operational parameters can be adjusted via headers and defines:

| Parameter | File Location | Default Value | Description |
|:---|:---|:---|:---|
| `WORKER_ID` | `node_src/main.cpp` | `1` | Individual identifier for the worker unit |
| `HUB_MAC` | `node_src/main.cpp` | `{0xE4, 0x65, ...}` | MAC address of the target Admin Hub |
| `TEMP_OFFSET` | `node_src/main.cpp` | `4.5` | Offset (°C) to compensate for MQ-2 heater radiation |
| `WORKER_COUNT` | `admin_src/main.cpp` | `2` | Number of worker units registered in Hub memory |
| `AP_SSID` | `admin_src/main.cpp` | `"SENTINEL-HUB"` | Wi-Fi network name broadcast by the Hub |
| `AP_PASS` | `admin_src/main.cpp` | `"sentinel123"` | WPA2 password for the Hub Access Point |
| `ESP_NOW_CHANNEL`| `admin_src/packet_defs.h` | `1` | Shared 2.4 GHz 802.11 channel |
| `TIMEOUT_MS` | `admin_src/main.cpp` | `10000` | Inactivity interval before node is flagged offline |

---

## Project Structure

```
sentinel/
├── admin_src/                 # Admin Hub ESP32 firmware
│   ├── main.cpp               #   ESP-NOW ingestion, SoftAP, WebSockets & LittleFS server
│   └── packet_defs.h          #   Shared packed structs & checksum verification
├── node_src/                  # Worker Wearable ESP32 firmware
│   ├── main.cpp               #   Sensor reading, 4-stage fall state machine, edge safety
│   └── packet_defs.h          #   Shared packed structs & checksum verification
├── docs/                      # Embedded web dashboard (served via LittleFS)
│   ├── index.html             #   UI markup, cards & gauge SVG templates
│   ├── style.css              #   Dark industrial styling & animations
│   └── app.js                 #   WebSocket client, sparklines, event log & mock engine
├── images/                    # Hardware prototype and UI screenshots
│   ├── Emergency.png          #   Dashboard emergency alert state
│   ├── Normal.png             #   Dashboard nominal operation
│   ├── Perfboard.jpeg         #   Hardware assembly & soldering
│   ├── Side View.png          #   Enclosure profile
│   └── Top View.png           #   Enclosure top view
├── .github/                   # GitHub templates & community config
│   ├── ISSUE_TEMPLATE/        #   Issue forms for bug reports & feature requests
│   └── pull_request_template.md
├── CHANGELOG.md               # Version history and release notes
├── CODE_OF_CONDUCT.md         # Contributor Covenant 2.1
├── CONTRIBUTING.md            # Build instructions, workflow & style guide
├── LICENSE                    # Apache License 2.0
├── README.md                  # Project overview & documentation
└── SECURITY.md                # Vulnerability reporting procedure
```

---

## Documentation

- [Contributing Guidelines](CONTRIBUTING.md) — Toolchain setup, flashing guide, coding standards, and PR process.
- [Security Policy](SECURITY.md) — Supported versions and private vulnerability reporting procedure.
- [Changelog](CHANGELOG.md) — Release notes and history.
- [Code of Conduct](CODE_OF_CONDUCT.md) — Community standards.

---

## Contributing

Contributions are welcome under the Apache 2.0 License. Please check [CONTRIBUTING.md](CONTRIBUTING.md) and our [Code of Conduct](CODE_OF_CONDUCT.md) before opening issues or submitting pull requests.

- [Open a Bug Report](https://github.com/asifahamed-ece/sentinel/issues/new/choose)
- [Submit a Feature Request](https://github.com/asifahamed-ece/sentinel/issues/new/choose)

---

## License

This project is licensed under the **Apache License 2.0** — see the [LICENSE](LICENSE) file for details.

```
Copyright 2026 Team Core Dumped

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0
```

---

## Team & Credits

Built by **Team Core Dumped**  
*Asif Ahamed S, Akshaya Kumar P, Sarvesh P*  
Department of Electronics & Communication Engineering, Rajalakshmi Engineering College  

© 2026 SENTINEL v6.0 · Licensed under [Apache-2.0](LICENSE)

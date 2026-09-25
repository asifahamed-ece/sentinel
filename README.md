# 🛡️ SENTINEL – Smart Emergency & Health Tracking System

> Real-time wearable IoT safety monitor for industrial workers. Low-cost, offline-ready, and engineered for rapid emergency response using bidirectional ESP-NOW communication.

[![License: Apache-2.0](https://img.shields.io/badge/License-Apache--2.0-blue.svg)](LICENSE)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](./)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)

## 📸 Project Gallery

| Physical Wearable Prototype | Live Dashboard & Analytics | Emergency Alert & Bidirectional Comms |
|:-------------------------:|:--------------------------:|:-----------------------------------:|
| ![Wearable Prototype](images/Top%20View.png) | ![Dashboard View](images/Emergency.png) | ![Side View](images/Side%20View.png) |

## ✨ Key Features

- 🆘 **4-Stage Fall Detection:** `Free Fall → Impact → Watching Recovery → Confirmed` (eliminates false positives)
- 🔄 **Full-Duplex ESP-NOW:** Collision-free, dynamic peer registration, <10ms transmission latency
- 🔔 **Persistent "Sticky SOS":** Emergency alerts remain active until manually acknowledged on the dashboard or physically reset by the worker
- 📊 **Live Industrial Dashboard:** Real-time sparklines, circular dose gauges, shift analytics, audio-visual alarms, and event logging
- 🌡️ **Multi-Sensor Monitoring:** Gas (MQ-2), Temperature/Humidity (DHT22), HR/SpO₂ (MAX30102), Acceleration (MPU6050)
- 💡 **Edge Safety Logic:** Status computation runs on-device; alerts trigger instantly even if the link drops

## 🛠️ Technology Stack

| Layer | Technologies |
|:---|:---|
| **Hardware** | ESP32, MPU6050, DHT22, MQ-2, MAX30102, SSD1306 OLED, Buzzer/LEDs |
| **Firmware** | C/C++ (Arduino), ESP-NOW, AsyncWebServer, ArduinoJson, `esp_task_wdt` |
| **Dashboard** | Vanilla JS, WebSockets, SVG Gauges/Sparklines, Web Audio API, CSS3 Animations |
| **Architecture** | Master-Slave IoT, Edge Processing, CSMA/CA + MAC-ACK retransmission |

## 📡 How It Works

1. **Worker Node** continuously samples sensors & computes safety status locally.
2. Telemetry streams to the **Admin Hub** via ESP-NOW (`500ms` in emergency, `2s` normal).
3. Hub aggregates data, manages peer registry, and broadcasts to the web dashboard via **WebSockets**.
4. Admin can trigger inline alerts or acknowledge emergencies; commands route back to the specific worker via ESP-NOW.

## ⚙️ Quick Setup

1. Flash [`admin_src/main.cpp`](./admin_src/main.cpp) to the Hub ESP32 and [`node_src/main.cpp`](./node_src/main.cpp) to each worker node (with `packet_defs.h` alongside). See [CONTRIBUTING.md](./CONTRIBUTING.md) for toolchain and pin setup.
2. Upload `index.html`, `style.css`, and `app.js` from [`docs/`](./docs) to the Hub's ESP32 LittleFS.
3. Connect to `SENTINEL-HUB` WiFi, open `http://192.168.4.1` in any browser.
4. Power on workers – they auto-register and begin streaming telemetry.

## 📖 Documentation & References

- 📄 The project report & presentation deck are available from the [team below](#-team--credits) on request (not tracked in this repository)
- 🔬 Based on research in wearable IoT safety systems & ESP-NOW field performance analysis
- 📚 Referenced: IEEE Sensors Journal, IndiaSpend Industrial Safety Reports, WONS 2025
- 🔒 See [SECURITY.md](./SECURITY.md) for supported versions and how to report vulnerabilities

## 🤝 Contributing

Contributions are welcome! Please read our [Contributing Guidelines](./CONTRIBUTING.md) and [Code of Conduct](./CODE_OF_CONDUCT.md) first.

- 🐛 Found a bug? [Open an issue](https://github.com/asifahamed-ece/sentinel/issues/new/choose)
- 💡 Have an idea? [Request a feature](https://github.com/asifahamed-ece/sentinel/issues/new/choose)
- 📝 Review the [Changelog](./CHANGELOG.md) for release history

## ⚖️ License

This project is licensed under the **Apache License 2.0** — see the [LICENSE](./LICENSE) file.

Copyright © 2026 **Team Core Dumped**. Open for academic, research, and SME deployment showcase use.

## 👥 Team & Credits

Built by **Team Core Dumped**  
🎓 *Asif Ahamed S, Akshaya Kumar P, Sarvesh P*  
Department of Electronics & Communication Engineering, Rajalakshmi Engineering College  
© 2026 SENTINEL v6.0 · Licensed under [Apache-2.0](./LICENSE)

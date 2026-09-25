# Contributing to SENTINEL

Thanks for your interest in contributing! SENTINEL is an open-source
smart emergency & health tracking system for industrial workers. Whether
you're fixing a bug, adding a sensor, improving the dashboard, or
polishing the docs — every contribution is welcome.

Please take a moment to read this guide and our
[Code of Conduct](CODE_OF_CONDUCT.md) before opening an issue or pull
request. All contributions are made under the
[Apache License 2.0](LICENSE).

## Table of Contents

- [How to contribute](#how-to-contribute)
- [Project structure](#project-structure)
- [Development environment](#development-environment)
  - [Option A: PlatformIO](#option-a-platformio)
  - [Option B: Arduino IDE](#option-b-arduino-ide)
- [Building & flashing](#building--flashing)
  - [Admin Hub firmware](#admin-hub-firmware)
  - [Worker Node firmware](#worker-node-firmware)
- [Uploading the web dashboard](#uploading-the-web-dashboard)
- [Code style](#code-style)
- [Commit message guidelines](#commit-message-guidelines)
- [Opening a pull request](#opening-a-pull-request)
- [Reporting bugs](#reporting-bugs)

## How to contribute

1. **Find or open an issue** — check the [issues](https://github.com/asifahamed-ece/sentinel/issues) tab first
   to avoid duplicates. Give the issue a clear, descriptive title.
2. **Fork the repository** and create a feature branch:
   `git checkout -b feat/my-feature`.
3. **Make your changes**, following this guide.
4. **Test your changes** on hardware if possible (see below).
5. **Open a pull request** against `main` and reference the issue number.

## Project structure

```
sentinel/
├── admin_src/          # Admin Hub firmware (ESP32)
│   ├── main.cpp        #   Hub: ESP-NOW aggregation, peer registry, WebSocket broadcast
│   └── packet_defs.h   #   Shared packet structs + checksum helpers
├── node_src/           # Worker Node firmware (ESP32)
│   ├── main.cpp        #   Node: sensor sampling, fall detection, edge safety logic
│   └── packet_defs.h   #   Shared packet structs + checksum helpers
├── docs/               # Web dashboard served from the Hub's LittleFS
│   ├── index.html      #   Dashboard markup
│   ├── style.css       #   Dashboard styling / animations
│   └── app.js          #   WebSocket client, gauges, sparklines, alarms
└── images/             # Project gallery assets
```

- `admin_src/packet_defs.h` and `node_src/packet_defs.h` **must stay in
  sync** — they define the 55-byte `SentinelPacket` and 20-byte
  `HubCommand` that travel over ESP-NOW. Keep both copies identical.
- The dashboard code has **no build step** — it runs from ESP32 LittleFS
  and must work in any modern browser (vanilla JS only).

## Development environment

### Option A: PlatformIO

A `platformio.ini` is not committed to the repo, so create a project per
target:

```ini
[env:admin-hub]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    bblanchon/ArduinoJson
    me-no-dev/ESPAsyncWebServer
    me-no-dev/AsyncTCP

[env:worker-node]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    adafruit/Adafruit SSD1306
    adafruit/Adafruit MPU6050
    adafruit/Adafruit Sensor
    adafruit/Adafruit GFX Library
    adafruit/DHT sensor library
```

Place the corresponding source (`admin_src/main.cpp` or
`node_src/main.cpp` + `packet_defs.h`) in `src/`, then
`pio run -t upload`.

### Option B: Arduino IDE

1. Install the **esp32** core via Boards Manager
   (`esp32 by Espressif Systems`).
2. Install the libraries listed in the table below via the Library
   Manager.
3. Rename the desired firmware file to `Admin.ino` / `Worker.ino`
   (Arduino requires the folder and `.ino` file to share a name).
4. Select *DOIT ESP32 DEVKIT V1* (or your board), plug in the device,
   and click *Upload*.

| Library | Used by |
|:--|:--|
| `ArduinoJson` (bblanchon) | Hub — JSON WebSocket payloads |
| `ESPAsyncWebServer` + `AsyncTCP` | Hub — serving the dashboard |
| `Adafruit SSD1306`, `Adafruit GFX` | Node — OLED display |
| `Adafruit MPU6050`, `Adafruit Sensor` | Node — accelerometer/fall detection |
| `DHT sensor library` | Node — temperature & humidity |

## Building & flashing

### Admin Hub firmware

- Edit `AP_SSID` / `AP_PASS` in `admin_src/main.cpp` if you change the
  access-point credentials.
- `WORKER_COUNT` should match the number of worker nodes you plan to run.
- Flash **one** ESP32 as the Hub; it registers peers dynamically over
  ESP-NOW.

### Worker Node firmware

- Set `WORKER_ID` in `node_src/main.cpp` — this **must be unique** per node.
- Set `HUB_MAC` to the MAC address from your programmed Hub
  (`{0xE4, 0x65, ...}` in the source is the default for development).
- Optionally adjust `TEMP_OFFSET` if the MQ2 heater skews the DHT22
  reading on your board layout.

### Wiring reference

Node pin assignments are documented in the `// ---------- PINS ----------`
block at the top of `node_src/main.cpp`. Keep pin changes accompanied by
the corresponding define.

## Uploading the web dashboard

The dashboard files in `docs/` are served from the Hub's **LittleFS**:

1. Flash the Hub firmware first.
2. Use the Arduino *ESP32 Sketch Data Upload* plugin or
   `pio run -t uploadfs` to push `docs/index.html`, `docs/style.css`,
   and `docs/app.js` to LittleFS.
3. When changing dashboard code, rebuild and re-upload the filesystem.

If you change the dashboard, verify:

- WebSocket updates render in **multiple browsers** (Chrome/Firefox).
- The audio-visual alarm (`Web Audio API`) works and fallback text alarms
  are visible.
- It remains dependency-free (no external CDNs/frameworks).

## Code style

Match the existing patterns — readability over cleverness:

- **Firmware (C++)**
  - 2-space indentation, 80-column lines.
  - Use `#define` for magic numbers and pins; keep the comment `// ---------- SECTION ----------` style.
  - Prefer named enums (e.g., `FallState`) over raw integers.
  - The packet structs are `#pragma pack(1)` — never change layout without
    updating **both** copies of `packet_defs.h` **and** the `static_assert`
    size checks.
- **Dashboard (HTML/CSS/JS)**
  - Vanilla JS only — no framework, no build step.
  - 2-space indentation; descriptive naming for gauges/sparklines.
  - CSS custom properties for theme colors; respect `prefers-reduced-motion`.
- **Docs** — keep Markdown tables and headers consistent with the README.

Do not add SPDX headers, licenses, or copyright lines to files beyond
what already exists, unless you are the copyright owner. Copyright lines
live in the project-level `LICENSE`.

## Commit message guidelines

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <subject>

<body>
```

- `feat` — new feature or sensor
- `fix` — bug fix
- `docs` — documentation only
- `style` — formatting, no behavior change
- `refactor` — internal change, no behavior change
- `test` — adding/updating tests
- `chore` — tooling, CI, maintenance

Examples:

```
feat(node): add CO2 sensor support to worker node
fix(hub): re-send unacknowledged alerts every 5s
docs: document LittleFS dashboard upload step
```

Keep the subject under 72 characters, use the imperative mood, and
reference the issue in the body when relevant (e.g., `Closes #12`).

## Opening a pull request

1. Rebase your branch onto the latest `main` before opening the PR.
2. Use the pull request template. Link the issue you're addressing.
3. Describe the hardware you tested on (board, sensors, firmware version).
4. If a change touches `packet_defs.h`, state explicitly that both copies
   were updated — a mismatch silently corrupts the wireless link.
5. Be responsive to review comments; keep the discussion constructive.

## Reporting bugs

Before reporting, please:

- Search the [issues](https://github.com/asifahamed-ece/sentinel/issues) for an existing report.
- Retest with a clean build and fresh LittleFS upload.
- Include: board model, firmware version, sensor list, exact steps,
  expected vs. actual behavior, and serial output if available.

Security vulnerabilities should **not** be reported as public issues —
see [SECURITY.md](SECURITY.md) for the private reporting process.
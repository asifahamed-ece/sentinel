# Changelog

All notable changes to SENTINEL are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Open-source release documentation: `LICENSE` (Apache-2.0),
  `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, and
  repository issue/PR templates.

## [6.0.0] - 2026

Initial open-source release of the SENTINEL system.

### Added

- **Admin Hub firmware** (`admin_src/`): full-duplex ESP-NOW aggregation,
  dynamic peer registration, WebSocket + littlefs web server broadcast,
  manual ACK, and inline alert routing.
- **Worker Node firmware** (`node_src/`): multi-sensor sampling
  (MPU6050, DHT22, MQ-2, MAX30102), 4-stage fall detection
  (`Free Fall → Impact → Watching Recovery → Confirmed`), edge safety
  logic with instant on-device alerts, panic button, and zone menu.
- **Shared packet protocol** (`packet_defs.h`): packed 55-byte
  `SentinelPacket` and 20-byte `HubCommand` with static size assertions
  and checksum verification.
- **Web dashboard** (`docs/`): live sparklines, circular dose gauges,
  shift analytics, audio-visual alarms, persistent "sticky SOS" alerts,
  and event logging over WebSockets.
- **Documentation & gallery**: project README, report/reference material,
  and hardware prototype images (`images/`).

[unreleased]: https://github.com/asifahamed-ece/sentinel/compare/v6.0.0...HEAD
[6.0.0]: https://github.com/asifahamed-ece/sentinel/releases/tag/v6.0.0
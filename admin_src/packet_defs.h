// ============================================================
//  SENTINEL — Shared Packet Definitions v1.0 (Final Synced)
//  Team Core Dumped © 2026
// ============================================================
#pragma once
#include <Arduino.h>

// Define expected sizes based on packed layout (pack(1))
// Calculated size for SentinelPacket with pack(1):
// uint8_t (1) + uint8_t (1) + char[20] (20) + float (4) + bool (1) + bool (1) + float (4) + float (4) + float (4) + float (4) + float (4) + uint8_t (1) + uint8_t (1) + uint8_t (1) + uint8_t (1) + bool (1) + uint8_t (1) + uint8_t (1)
// Total = 1+1+20+4+1+1+4+4+4+4+4+1+1+1+1+1+1+1 = 55 bytes
//
// Calculated size for HubCommand with pack(1):
// uint8_t (1) + uint8_t (1) + uint8_t (1) + uint8_t (1) + char[16] (16)
// Total = 1+1+1+1+16 = 20 bytes

#pragma pack(push, 1) // Push current pack setting and set to 1-byte alignment
typedef struct {
  uint8_t  workerID;        // 1 byte
  uint8_t  zoneID;          // 1 byte
  char     zoneName[20];    // 20 bytes
  float    totalAccel;      // 4 bytes
  bool     fallDetected;    // 1 byte
  bool     gasDetected;     // 1 byte
  float    tempC;           // 4 bytes
  float    humidity;        // 4 bytes
  float    soundDB;         // 4 bytes
  float    noiseDosePct;    // 4 bytes
  float    heatDosePct;     // 4 bytes
  uint8_t  heartRate;       // 1 byte
  uint8_t  spO2;            // 1 byte
  uint8_t  safetyStatus;    // 1 byte
  uint8_t  emergencyType;   // 1 byte
  bool     panicPressed;    // 1 byte
  uint8_t  batteryPct;      // 1 byte
  uint8_t  fallStateCode;   // 1 byte
  // Total = 1+1+20+4+1+1+4+4+4+4+4+1+1+1+1+1+1+1 = 55 bytes
} SentinelPacket;
#pragma pack(pop) // Pop the previous pack setting

#pragma pack(push, 1) // Push current pack setting and set to 1-byte alignment
typedef struct {
  uint8_t  targetWorkerID;  // 1 byte
  uint8_t  messageType;     // 1 byte
  uint8_t  priority;        // 1 byte
  uint8_t  checksum;        // 1 byte
  char     customMsg[16];   // 16 bytes
  // Total = 1+1+1+1+16 = 20 bytes
} HubCommand;
#pragma pack(pop) // Pop the previous pack setting

// --- CORRECTED STATIC ASSERTIONS ---
// These reflect the actual calculated sizes based on the provided struct definitions and pack(1).
static_assert(sizeof(SentinelPacket) == 55, "SentinelPacket size mismatch with pack(1)");
static_assert(sizeof(HubCommand) == 20, "HubCommand size mismatch with pack(1)");

// --- Constants ---
#define MSG_ACK_RESCUE    0
#define MSG_ALERT_GAS     1
#define MSG_ALERT_NOISE   2
#define MSG_ALERT_HEAT    3
#define MSG_ALERT_GENERAL 4

#define PRIORITY_INFO     1
#define PRIORITY_WARN     2
#define PRIORITY_CRITICAL 3

#define ESP_NOW_CHANNEL   1
#define CMD_TIMEOUT_MS    30000

// --- Checksum Functions ---
inline uint8_t computeChecksum(const HubCommand* cmd) {
  uint8_t cs = cmd->targetWorkerID ^ cmd->messageType ^ cmd->priority;
  for (int i = 0; i < 16; i++) {
      cs ^= cmd->customMsg[i];
  }
  return cs;
}

inline bool verifyChecksum(const HubCommand* cmd) {
  return cmd->checksum == computeChecksum(cmd);
}
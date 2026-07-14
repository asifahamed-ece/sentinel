// ============================================================
//  SENTINEL Admin Hub — Full Duplex, Manual ACK, Inline Alerts
//  Fixed: dynamic peer, debug prints, channel enforcement
// ============================================================
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "packet_defs.h"

#define AP_SSID        "SENTINEL-HUB"
#define AP_PASS        "sentinel123"
#define BUZZER_PIN     2
#define WORKER_COUNT   2
#define TIMEOUT_MS     10000
#define HEARTBEAT_MS   3000
#define JSON_BUF_SIZE  1536

struct WorkerState {
  SentinelPacket data;
  uint8_t        mac[6] = {0};
  bool           active = false;
  unsigned long  lastSeen = 0;
  char           alertMsg[40] = "None";
  int8_t         rssi = 0;
  bool           ackSent = false;
  bool           ackPending = false;
};

WorkerState workers[WORKER_COUNT];
bool hubAlarm = false;
int totalPackets = 0;
unsigned long lastWsSend = 0;

const char* SLABEL[]  = { "SAFE", "WARN", "DANGER", "EMERGENCY" };
const char* ELABEL[]  = { "None", "Fall", "Panic", "Gas", "Heat", "Noise", "Health" };
const char* FSLABEL[] = { "No Fall", "Free Fall", "Impact", "Watching", "CONFIRMED" };

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ── Buzzer ─────────────────────────────────────────────────
void beep(int n, int on=100, int off=80) {
  for (int i=0; i<n; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(on);
    digitalWrite(BUZZER_PIN, LOW); delay(off);
  }
}

void alarmTick() {
  static unsigned long t=0; static int phase=0;
  const int pat[] = {150,100,150,100,150,900};
  const bool st[] = {true,false,true,false,true,false};
  if (millis()-t >= (unsigned long)pat[phase]) {
    t = millis();
    digitalWrite(BUZZER_PIN, st[phase]);
    phase = (phase+1)%6;
  }
}

// ── Dynamic peer registration ──────────────────────────────
void addPeerIfNew(const uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = ESP_NOW_CHANNEL;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) == ESP_OK) {
    Serial.printf("[PEER] Added %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  } else {
    Serial.println("[PEER] Add failed");
  }
}

// ── JSON Builder ───────────────────────────────────────────
String buildJSON() {
  StaticJsonDocument<2048> doc; // Increased buffer size
  doc["packets"] = totalPackets;
  doc["uptime"] = millis()/1000;
  doc["alarm"] = hubAlarm;
  
  JsonArray arr = doc.createNestedArray("workers");
  for (int i=0; i<WORKER_COUNT; i++) {
    JsonObject w = arr.createNestedObject();
    WorkerState& ws_ = workers[i];
    w["id"] = i+1;
    w["active"] = ws_.active;
    if (!ws_.active) continue;
    
    SentinelPacket& p = ws_.data;
    uint8_t sc = min((int)p.safetyStatus, 3);
    uint8_t et = min((int)p.emergencyType, 6);
    uint8_t fc = min((int)p.fallStateCode, 4);
    
    // CRITICAL: Ensure these keys match app.js exactly
    w["zone"] = String(p.zoneName);
    w["statusCode"] = sc;
    w["status"] = String(SLABEL[sc]);
    w["emergType"] = et;
    w["emerg"] = String(ELABEL[et]);
    w["alertMsg"] = String(ws_.alertMsg);
    w["lastSeen"] = (millis()-ws_.lastSeen)/1000;
    
    w["temp"] = p.tempC;
    w["hum"] = p.humidity;
    w["gas"] = p.gasDetected;
    w["soundDB"] = p.soundDB;
    w["hr"] = p.heartRate;
    w["spo2"] = p.spO2;
    w["accel"] = p.totalAccel;
    w["fall"] = p.fallDetected;
    w["fallState"] = String(FSLABEL[fc]);
    w["fallStateCode"] = fc;
    w["panic"] = p.panicPressed;
    w["battery"] = p.batteryPct;
    
    // Ensure Dose values are sent
    w["noiseDose"] = p.noiseDosePct; 
    w["heatDose"] = p.heatDosePct;
    w["rssi"] = ws_.rssi;
    w["ackSent"] = ws_.ackSent;
  }
  
  String out;
  serializeJson(doc, out);
  return out;
}

bool sendCommandToWorker(uint8_t workerID, uint8_t msgType, uint8_t priority, const char* customMsg) {
  // ... (identical to earlier, but ensure addPeerIfNew is called)
  if (workerID<1 || workerID>WORKER_COUNT) return false;
  WorkerState& w = workers[workerID-1];
  if (!w.active) {
    Serial.printf("[CMD] Worker %d inactive\n", workerID);
    return false;
  }
  bool macValid = true;
  for (int i=0; i<6; i++) if (w.mac[i]==0) macValid=false;
  if (!macValid) {
    Serial.printf("[CMD] Invalid MAC for W%d\n", workerID);
    return false;
  }

  HubCommand cmd;
  memset(cmd.customMsg, 0, sizeof(cmd.customMsg));   // ← ZERO the buffer first
  cmd.targetWorkerID = workerID;
  cmd.messageType = msgType;
  cmd.priority = priority;
  strncpy(cmd.customMsg, customMsg ? customMsg : "", sizeof(cmd.customMsg)-1);
  cmd.customMsg[sizeof(cmd.customMsg)-1] = '\0';
  cmd.checksum = computeChecksum(&cmd);

  esp_err_t result = esp_now_send(w.mac, (uint8_t*)&cmd, sizeof(cmd));
  if (result == ESP_OK) {
    Serial.printf("[CMD] Sent to W%d: type=%d msg='%s'\n", workerID, msgType, cmd.customMsg);
    return true;
  } else {
    Serial.printf("[CMD] Send failed to W%d (err=%d)\n", workerID, result);
    return false;
  }
}

// ── ESP‑NOW Receive ────────────────────────────────────────
void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  if (len != sizeof(SentinelPacket)) {
    Serial.printf("[ESP-NOW] Bad pkt size: %d, expected %d\n", len, sizeof(SentinelPacket));
    return;
  }
  totalPackets++;

  SentinelPacket pkt;
  memcpy(&pkt, data, sizeof(pkt));
  uint8_t idx = pkt.workerID - 1;
  if (idx >= WORKER_COUNT) {
    Serial.printf("[ESP-NOW] Invalid workerID %d\n", pkt.workerID);
    return;
  }

  addPeerIfNew(mac);   // ★ Register worker as peer for future commands

  memcpy(workers[idx].mac, mac, 6);
  workers[idx].data = pkt;
  workers[idx].active = true;
  workers[idx].lastSeen = millis();
  workers[idx].ackPending = true;

  // Set alert message
  if (pkt.safetyStatus >= 2) {
    switch (pkt.emergencyType) {
      case 1: strncpy(workers[idx].alertMsg,"FALL DETECTED",39); break;
      case 2: strncpy(workers[idx].alertMsg,"PANIC - SOS",39); break;
      case 3: strncpy(workers[idx].alertMsg,"GAS DETECTED",39); break;
      case 4: strncpy(workers[idx].alertMsg,"HEAT CRITICAL",39); break;
      case 5: strncpy(workers[idx].alertMsg,"NOISE LIMIT",39); break;
      case 6: strncpy(workers[idx].alertMsg,"HEALTH ALERT",39); break;
      default: strncpy(workers[idx].alertMsg,"DANGER",39);
    }
  } else {
    strncpy(workers[idx].alertMsg,"None",39);
  }

  // Recompute hub alarm
  hubAlarm = false;
  for (int i=0; i<WORKER_COUNT; i++)
    if (workers[i].active && workers[i].data.safetyStatus>=2) hubAlarm = true;
  if (!hubAlarm) digitalWrite(BUZZER_PIN, LOW);

  // Print to serial
  uint8_t sc = min((int)pkt.safetyStatus, 3);
  uint8_t fc = min((int)pkt.fallStateCode, 4);
  Serial.printf("[PKT #%d] W%d %s T:%.1fC Gas:%s FS:%s ACK:%s\n",
                totalPackets, pkt.workerID, SLABEL[sc], pkt.tempC,
                pkt.gasDetected ? "YES" : "no", FSLABEL[fc],
                workers[idx].ackSent ? "sent" : "pending");

  ws.textAll(buildJSON());
  lastWsSend = millis();
}

void setupWebSocket() {
  ws.onEvent([](AsyncWebSocket* svr, AsyncWebSocketClient* client,
                AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      client->text(buildJSON());
    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index==0 && info->len==len && info->opcode==WS_TEXT) {
        data[len] = 0;
        StaticJsonDocument<512> doc;
        if (deserializeJson(doc, (char*)data)) return;

        const char* cmd = doc["command"];
        if (cmd && strcmp(cmd,"sendAlert")==0) {
          uint8_t wid = doc["workerId"];
          uint8_t alertType = doc["alertType"];
          const char* msg = doc["message"] | "";
          uint8_t priority = doc["priority"] | PRIORITY_WARN;
          bool ok = sendCommandToWorker(wid, alertType, priority, msg);
          StaticJsonDocument<256> resp;
          resp["commandResponse"]["status"] = ok ? "sent" : "failed";
          resp["commandResponse"]["workerId"] = wid;
          String out; serializeJson(resp, out);
          client->text(out);
        }
        else if (cmd && strcmp(cmd,"manualAck")==0) {
          uint8_t wid = doc["workerId"];
          if (wid>=1 && wid<=WORKER_COUNT) {
            workers[wid-1].ackPending = false;
            workers[wid-1].ackSent = true;
            sendCommandToWorker(wid, MSG_ACK_RESCUE, PRIORITY_CRITICAL, "RESCUE ACK");
            StaticJsonDocument<256> resp;
            resp["commandResponse"]["status"] = "sent";
            resp["commandResponse"]["workerId"] = wid;
            String out; serializeJson(resp, out);
            client->text(out);
          }
        }
      }
    }
  });
  server.addHandler(&ws);
}

void setup() {
  Serial.begin(115200); delay(500);
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);

  if (!LittleFS.begin(true)) {
    Serial.println("[LittleFS] FAILED");
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS, ESP_NOW_CHANNEL);
  esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  Serial.printf("[AP] %s | IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  Serial.print("[HUB MAC] ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init FAILED"); while(1);
  }
  esp_now_register_recv_cb(onReceive);

  setupWebSocket();
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.onNotFound([](AsyncWebServerRequest* req){
    req->send(404, "text/plain", "Not found");
  });
  server.begin();

  beep(2);
  Serial.println("[HUB] Ready");
}

void loop() {
  // Run alarm pattern if active
  if (hubAlarm) alarmTick();
  static unsigned long lastHB = 0;
  unsigned long now = millis();

  if (now - lastHB >= HEARTBEAT_MS) {
    lastHB = now;
    ws.cleanupClients();
    // 1. Timeout check
    for (int i = 0; i < WORKER_COUNT; i++) {
      if (workers[i].active && (now - workers[i].lastSeen > TIMEOUT_MS)) {
        workers[i].active = false;
        workers[i].ackSent = false;
        workers[i].ackPending = false;          // ⬅️ Clear phantom alerts
        strncpy(workers[i].alertMsg, "None", 39); // Reset message
        Serial.printf("[TIMEOUT] W%d offline\n", i + 1);
      }
    }
    // 2. Recompute hub alarm (only stays TRUE until ACK is pressed)
    hubAlarm = false;
    for (int i = 0; i < WORKER_COUNT; i++) {
      if (workers[i].active && workers[i].data.safetyStatus >= 2 && workers[i].ackPending) {
        hubAlarm = true;
        break; // Optimization: stop checking once one emergency is found
      }
    }
    // 3. Kill buzzer if no pending emergencies
    if (!hubAlarm) digitalWrite(BUZZER_PIN, LOW);
    // 4. Push state to dashboard every 2.5s
    if (now - lastWsSend >= 2500) {
      ws.textAll(buildJSON());
      lastWsSend = now;
    }
  }
}
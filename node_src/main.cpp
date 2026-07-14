// ============================================================
//  SENTINEL Worker Node v6.4 — Fixed OLED & Alert Beep
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include "packet_defs.h"

// ---------- PINS ----------
#define I2C_SDA      21
#define I2C_SCL      22
#define OLED_ADDR    0x3C
#define DHT22_PIN    4
#define MQ2_DPIN     33
#define BTN_PANIC    16
#define BTN_RESET    5
#define BTN_ZONE     27
#define LED_GREEN    2
#define LED_RED      12
#define LED_YELLOW   13
#define BUZZER_PIN   14
#define TEMP_OFFSET 4.5  // Compensates for MQ2 heater radiation. Adjust ±0.5 if needed.

// ---------- IDENTITY ----------
#define WORKER_ID   1          // Change for each worker
const char* ZONES[] = {"Chemical Store","Assembly Line","CNC Bay","Welding Area","Loading Dock","Break Room"};
uint8_t zone = 3;

// ---------- HUB MAC ----------
uint8_t HUB_MAC[] = {0xE4, 0x65, 0xB8, 0xE7, 0x1E, 0x8C};   // Replace with your hub's MAC

// ---------- GLOBALS ----------
Adafruit_SSD1306 oled(128,64,&Wire,-1);
Adafruit_MPU6050 mpu;
DHT dht(DHT22_PIN, DHT22);
SentinelPacket pkt = {};
HubCommand incomingCmd = {};
bool espNowOK = false;
uint32_t sendOK=0, sendFail=0;

struct AlertState {
  bool active = false;
  uint8_t type = 0;
  uint8_t priority = 0;
  unsigned long startTime = 0;
  char message[17] = "";
} currentAlert;

enum FallState { NO_FALL, FREE_FALL_DETECTED, IMPACT_DETECTED, WATCHING_RECOVERY, FALL_CONFIRMED };
FallState fallState = NO_FALL;
unsigned long fallStateStart = 0;

// ---------- FORWARD DECLARATIONS ----------
void oledClear();
void showBoot();
void showDash();
void showIncomingAlert(uint8_t type, const char* msg);
void showRescueACK();
void showEmergency(const char*,const char*);
void showWarning(const char*,const char*);
void showZoneMenu();
void playSoothingTone();
void playAlertBeep();  // NEW: Alert beep
void beep(int n, int on=100, int off=80);
void onReceive(const uint8_t* mac, const uint8_t* data, int len);
void espNowInit();
void sendPkt();
void readMPU();
void readDHT();
void readMQ2();
void readSimulatedMAX();
void readSimulatedSound();
void computeSafety();
void handleIncomingAlerts();
void handleZone();

// ---------- OLED FUNCTIONS ----------
void oledClear() {
  oled.clearDisplay(); oled.setTextColor(SSD1306_WHITE); oled.setTextSize(1); oled.setCursor(0,0);
}

void showBoot() {
  oledClear(); oled.setTextSize(2); oled.setCursor(14,4); oled.print("SENTINEL");
  oled.setTextSize(1); oled.setCursor(0,26); oled.print("Worker Safety System");
  oled.setCursor(0,38); oled.print("Node #"); oled.print(WORKER_ID); oled.print(" | v6.4");
  oled.setCursor(0,52); oled.print("Initializing..."); oled.display(); delay(2000);
}

void showDash() {
  oledClear();
  oled.setCursor(0,0); oled.print("S# "); oled.print(WORKER_ID); oled.print("  ");
  switch(pkt.safetyStatus) {
    case 0: oled.print("SAFE    "); break;
    case 1: oled.print("WARN    "); break;
    case 2: oled.print("DANGER  "); break;
    default: oled.print("EMERG   "); break;
  }
  oled.print("B:"); oled.print(pkt.batteryPct); oled.print("% ");
  oled.drawLine(0,10,127,10,SSD1306_WHITE);
  oled.setCursor(0,13); oled.print("T:"); oled.print(pkt.tempC,1); oled.print("C ");
  oled.print("H:"); oled.print(pkt.humidity,1); oled.print("% ");
  oled.setCursor(0,23);
  if(pkt.heartRate>0) { oled.print("HR:"); oled.print(pkt.heartRate); oled.print("bpm SpO2:"); oled.print(pkt.spO2); oled.print("%"); }
  else oled.print("HR:-- SpO2:--");
  oled.setCursor(0,33); oled.print("Gas:"); oled.print(pkt.gasDetected?"DETECTED":"SAFE     ");
  oled.setCursor(0,43); oled.print("Noise:"); oled.print((int)pkt.noiseDosePct); oled.print("% ");
  oled.print("Heat:"); oled.print((int)pkt.heatDosePct); oled.print("% ");
  oled.setCursor(0,53); oled.print("A:"); oled.print(pkt.totalAccel,2); oled.print("g  ");
  oled.print(ZONES[zone-1]); oled.display();
}

// FIXED: Replaced ⚠ with text and centered
void showIncomingAlert(uint8_t type, const char* msg) {
  oledClear(); oled.drawRect(0,0,128,64,SSD1306_WHITE); oled.drawRect(2,2,124,60,SSD1306_WHITE);
  oled.setTextSize(2); oled.setCursor(26,6); oled.print("ALERT");
  oled.setTextSize(1); oled.setCursor(4,28);
  switch(type) {
    case MSG_ALERT_GAS:   oled.print("GAS DETECTED"); break;
    case MSG_ALERT_NOISE: oled.print("HIGH NOISE"); break;
    case MSG_ALERT_HEAT:  oled.print("HEAT WARNING"); break;
    case MSG_ALERT_GENERAL: oled.print("ADMIN ALERT"); break;
    default: oled.print("SAFETY ALERT");
  }
  oled.setCursor(4,42); oled.print(msg[0]?msg:"Check conditions"); oled.display();
}

// FIXED: Replaced 🛟 with text and centered
void showRescueACK() {
  oledClear(); oled.drawRect(0,0,128,64,SSD1306_WHITE); oled.drawRect(2,2,124,60,SSD1306_WHITE);
  oled.setTextSize(2); oled.setCursor(20,10); oled.print("RESCUE");
  oled.setTextSize(1); oled.setCursor(4,36); oled.print("Help is coming!");
  oled.setCursor(4,48); oled.print("Stay calm & safe"); oled.display();
  playSoothingTone();
}

void showEmergency(const char* l1, const char* l2) {
  oledClear(); oled.drawRect(0,0,128,64,SSD1306_WHITE); oled.drawRect(2,2,124,60,SSD1306_WHITE);
  oled.setTextSize(2); oled.setCursor(4,6); oled.print("EMERGENCY");
  oled.setTextSize(1); oled.setCursor(4,32); oled.print(l1); oled.setCursor(4,46); oled.print(l2); oled.display();
}

void showWarning(const char* l1, const char* l2) {
  oledClear(); oled.drawRect(0,0,128,64,SSD1306_WHITE); oled.setCursor(4,4); oled.print("!! WARNING !!");
  oled.drawLine(0,14,127,14,SSD1306_WHITE); oled.setCursor(4,20); oled.print(l1); oled.setCursor(4,36); oled.print(l2); oled.display();
}

void showZoneMenu() {
  oledClear(); oled.setCursor(0,0); oled.print("SELECT ZONE:"); oled.drawLine(0,10,127,10,SSD1306_WHITE);
  oled.setCursor(0,18); oled.print("> "); oled.print(ZONES[zone-1]);
  oled.setCursor(0,50); oled.print("Hold 3s to confirm"); oled.display();
}

// ── Tones ──────────────────────────────────────────────────
void playSoothingTone() {
  tone(BUZZER_PIN, 988, 150); delay(180);
  tone(BUZZER_PIN, 1319, 150); delay(180);
  noTone(BUZZER_PIN);
}

void playAlertBeep() {
  // Triple beep pattern for admin alerts
  tone(BUZZER_PIN, 1200, 100); delay(150);
  tone(BUZZER_PIN, 1200, 100); delay(150);
  tone(BUZZER_PIN, 1400, 200); delay(100);
  noTone(BUZZER_PIN);
}

void beep(int n, int on, int off){
  for(int i=0;i<n;i++){ digitalWrite(BUZZER_PIN,HIGH); delay(on); digitalWrite(BUZZER_PIN,LOW); delay(off); }
}

// ---------- ESP‑NOW ----------
void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  if(len != sizeof(HubCommand)){ Serial.printf("[ESP-NOW] Bad cmd size: %d\n", len); return; }
  memcpy(&incomingCmd, data, sizeof(incomingCmd));
  if(!verifyChecksum(&incomingCmd)){ Serial.println("[CMD] Checksum FAIL"); return; }
  if(incomingCmd.targetWorkerID != WORKER_ID && incomingCmd.targetWorkerID != 0) return;
  
  Serial.printf("[CMD] RX: type=%d msg='%s'\n", incomingCmd.messageType, incomingCmd.customMsg);
  
  if(incomingCmd.messageType == MSG_ACK_RESCUE){
    currentAlert.active = true;
    currentAlert.type = MSG_ACK_RESCUE;
    currentAlert.startTime = millis();
    strncpy(currentAlert.message,"RESCUE ACK",16);
    pkt.safetyStatus = 2; pkt.emergencyType = 0;
    fallState = NO_FALL; pkt.fallDetected = false; pkt.panicPressed = false;
    showRescueACK();
  }
  else if(incomingCmd.messageType >= MSG_ALERT_GAS && incomingCmd.messageType <= MSG_ALERT_GENERAL){
    currentAlert.active = true;
    currentAlert.type = incomingCmd.messageType;
    currentAlert.priority = incomingCmd.priority;
    currentAlert.startTime = millis();
    strncpy(currentAlert.message, incomingCmd.customMsg, sizeof(currentAlert.message)-1);
    currentAlert.message[sizeof(currentAlert.message)-1] = '\0';
    
    // FIX: Play alert beep
    playAlertBeep();
    
    showIncomingAlert(incomingCmd.messageType, incomingCmd.customMsg);
  }
}

void espNowInit() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if(esp_now_init()!=ESP_OK){ Serial.println("[ESP-NOW] Init FAILED"); return; }
  esp_now_register_send_cb([](const uint8_t* mac, esp_now_send_status_t status){
    if(status==ESP_NOW_SEND_SUCCESS) sendOK++; else { sendFail++; Serial.printf("[ESP-NOW] Send FAIL #%u\n", sendFail); }
  });
  esp_now_register_recv_cb(onReceive);
  esp_now_peer_info_t peer;
  memset(&peer,0,sizeof(peer));
  memcpy(peer.peer_addr, HUB_MAC,6);
  peer.channel = ESP_NOW_CHANNEL;
  peer.encrypt = false;
  if(esp_now_add_peer(&peer)!=ESP_OK){ Serial.println("[ESP-NOW] Peer FAILED"); return; }
  espNowOK = true;
  Serial.println("[ESP-NOW] Ready");
}

void sendPkt() {
  if(!espNowOK) return;
  pkt.workerID = WORKER_ID;
  pkt.zoneID = zone;
  strncpy(pkt.zoneName, ZONES[zone-1], sizeof(pkt.zoneName)-1);
  pkt.zoneName[sizeof(pkt.zoneName)-1]='\0';
  esp_now_send(HUB_MAC,(uint8_t*)&pkt,sizeof(pkt));
}

// ---------- SENSORS ----------
void readMPU() {
  sensors_event_t a,g,temp; mpu.getEvent(&a,&g,&temp);
  float ax=a.acceleration.x, ay=a.acceleration.y, az=a.acceleration.z;
  pkt.totalAccel = sqrt(ax*ax + ay*ay + az*az) / 9.81;  // in g
  
  // Basic fall detection
  if(pkt.totalAccel < 0.3 && fallState==NO_FALL) { 
    fallState=FREE_FALL_DETECTED; 
    fallStateStart=millis(); 
  }
  else if(pkt.totalAccel > 3.0 && fallState==FREE_FALL_DETECTED && (millis()-fallStateStart<500)) {
    fallState=IMPACT_DETECTED; 
    fallStateStart=millis();
  }
  else if(fallState==IMPACT_DETECTED && (millis()-fallStateStart)>1000) { 
    fallState=WATCHING_RECOVERY; 
    fallStateStart=millis(); 
  }
  else if(fallState==WATCHING_RECOVERY && (millis()-fallStateStart)>3000 && pkt.totalAccel<1.5) { 
    fallState=FALL_CONFIRMED; 
  }
  
  // REMOVED: Auto-recovery line that was resetting fallState to NO_FALL
  // Now FALL_CONFIRMED persists until BTN_RESET or ACK
  
  pkt.fallDetected = (fallState==FALL_CONFIRMED);
  pkt.fallStateCode = (uint8_t)fallState;
}

void readDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();  
  if(!isnan(t)) pkt.tempC = t - TEMP_OFFSET;  // Apply compensation
  if(!isnan(h)) pkt.humidity = h;
  pkt.heatDosePct = constrain(map((int)pkt.tempC, 20, 40, 0, 100), 0, 100);
}

void readMQ2() {
  pkt.gasDetected = (digitalRead(MQ2_DPIN) == LOW);
}

void readSimulatedMAX() {
  static uint8_t hrBase=75, spO2Base=98;
  if(millis()%2000 < 10) { hrBase += random(-2,3); spO2Base += random(-1,2);
    hrBase = constrain(hrBase,50,150); spO2Base = constrain(spO2Base,90,100); }
  pkt.heartRate = hrBase + random(-3,4);
  pkt.spO2 = spO2Base + random(-1,2);
}

void readSimulatedSound() {
  static float currentSound = 65.0;
  // Aggressive fluctuation: Add random noise between -5 and +5
  currentSound += random(-50, 50) / 10.0;
  // Strictly clamp between 40 and 90
  if (currentSound > 90.0) currentSound = 90.0 - random(1, 5);
  if (currentSound < 40.0) currentSound = 40.0 + random(1, 5);
  pkt.soundDB = currentSound;
  // Map 40dB-90dB range to 0%-100% Dose
  pkt.noiseDosePct = map((int)currentSound, 40, 90, 0, 100);
}

// ---------- SAFETY ----------
void computeSafety() {
  pkt.safetyStatus = 0; pkt.emergencyType = 0;
  if(pkt.gasDetected) { pkt.safetyStatus=3; pkt.emergencyType=3; return; }
  if(pkt.panicPressed) { pkt.safetyStatus=3; pkt.emergencyType=2; return; }
  if(fallState==FALL_CONFIRMED) { pkt.safetyStatus=3; pkt.emergencyType=1; return; }
  
  bool warn=false, danger=false;
  if(pkt.tempC>45) danger=true; else if(pkt.tempC>40) warn=true;
  if(pkt.soundDB>95) danger=true; else if(pkt.soundDB>85) warn=true;
  if(pkt.heartRate>150||pkt.heartRate<45) danger=true; else if(pkt.heartRate>120||pkt.heartRate<55) warn=true;
  if(pkt.spO2<90) danger=true; else if(pkt.spO2<94) warn=true;
  
  if(danger){ pkt.safetyStatus=2; pkt.emergencyType=(pkt.tempC>45)?4:((pkt.soundDB>95)?5:6); }
  else if(warn) pkt.safetyStatus=1;
  
  // pkt.batteryPct = (millis()/10000) % 101;
  pkt.batteryPct = 80;  // Static 80% battery level
}

// ---------- HANDLERS ----------
void handleIncomingAlerts() {
  if(!currentAlert.active) return;
  if(millis()-currentAlert.startTime>CMD_TIMEOUT_MS){ currentAlert.active=false; return; }
  if(digitalRead(BTN_RESET)==LOW){ currentAlert.active=false; Serial.println("[ALERT] Reset"); }
}

void handleZone() {
  static unsigned long pressStart=0;
  bool pressed = (digitalRead(BTN_ZONE)==LOW);
  if(pressed&&pressStart==0) pressStart=millis();
  if(pressed&&pressStart>0){
    if(millis()-pressStart>=3000){ zone = (zone%6)+1; pressStart = millis(); showDash(); }
  }
  if(!pressed) pressStart=0;
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200); delay(2000);
  pinMode(LED_GREEN,OUTPUT); pinMode(LED_RED,OUTPUT); pinMode(LED_YELLOW,OUTPUT);
  pinMode(BUZZER_PIN,OUTPUT);
  pinMode(BTN_PANIC,INPUT_PULLUP); pinMode(BTN_RESET,INPUT_PULLUP); pinMode(BTN_ZONE,INPUT_PULLUP);
  
  Wire.begin(I2C_SDA,I2C_SCL);
  oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  showBoot();
  
  if(!mpu.begin()) Serial.println("[MPU] Failed");
  else Serial.println("[MPU] OK");
  
  dht.begin();
  pinMode(MQ2_DPIN, INPUT);
  espNowInit();
  
  esp_task_wdt_init(10,true); esp_task_wdt_add(NULL);
  beep(2); digitalWrite(LED_GREEN,HIGH);
  Serial.println("[WORKER] Ready");
}

// ---------- LOOP ----------
void loop() {
  unsigned long now = millis();
  static unsigned long tMPU = 0, tDHT = 0, tSIM = 0, tDisp = 0, tSend = 0, tPrint = 0;

  if (now - tMPU >= 20)   { readMPU(); tMPU = now; }
  if (now - tDHT >= 2000) { readDHT(); tDHT = now; }
  if (now - tSIM >= 100)  { readMQ2(); readSimulatedMAX(); readSimulatedSound(); tSIM = now; }
  
  computeSafety();
  esp_task_wdt_reset();
  handleIncomingAlerts();

  if (now - tPrint >= 3000) {
    Serial.printf("[PKT] S:%d E:%d T:%.1f H:%.1f HR:%d SpO2:%d Noise:%.1f Heat:%.1f Alert:%s\n",
      pkt.safetyStatus, pkt.emergencyType, pkt.tempC, pkt.humidity,
      pkt.heartRate, pkt.spO2, pkt.soundDB, pkt.heatDosePct, currentAlert.active ? "ACTIVE" : "none");
    tPrint = now;
  }

  if (now - tDisp >= 1000) {
    if (currentAlert.active) {
      if (currentAlert.type == MSG_ACK_RESCUE) showRescueACK();
      else showIncomingAlert(currentAlert.type, currentAlert.message);
    } else if (pkt.safetyStatus == 3) {
      const char* eMsg[] = { "", "FALL DETECTED", "PANIC - SOS", "GAS DETECTED",
        "HEAT CRITICAL", "NOISE LIMIT", "HEALTH ALERT" };
      showEmergency(eMsg[pkt.emergencyType], pkt.emergencyType == 1 ? "Press RESET if OK" : "Help on the way!");
    } else {
      showDash();
    }
    tDisp = now;
  }

  uint16_t interval = (pkt.safetyStatus >= 2) ? 500 : 2000;
  if (now - tSend >= interval) { sendPkt(); tSend = now; }

  bool adminAlertActive = (currentAlert.active && currentAlert.type != MSG_ACK_RESCUE);
  if (adminAlertActive) {
    static unsigned long ledTimer = 0; static bool ledOn = false;
    if (millis() - ledTimer > 500) { ledOn = !ledOn; ledTimer = millis(); }
    digitalWrite(LED_GREEN, ledOn); digitalWrite(LED_YELLOW, ledOn); digitalWrite(LED_RED, ledOn);
  } else {
    digitalWrite(LED_GREEN,   pkt.safetyStatus == 0 && !currentAlert.active);
    digitalWrite(LED_YELLOW, pkt.safetyStatus == 1);
    digitalWrite(LED_RED,    pkt.safetyStatus >= 2 || (currentAlert.active && currentAlert.priority >= PRIORITY_CRITICAL));
  }

  static unsigned long buzzerToggle = 0;
  if (pkt.safetyStatus == 3 && !currentAlert.active) {
    if (millis() - buzzerToggle >= 300) { digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN)); buzzerToggle = millis(); }
  } else if (currentAlert.active && currentAlert.type == MSG_ACK_RESCUE) {
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  static unsigned long panicT = 0;
  if (digitalRead(BTN_PANIC) == LOW) {
    if (panicT == 0) panicT = millis();
    if (millis() - panicT >= 1000) { pkt.panicPressed = true; Serial.println("[PANIC] SOS triggered"); }
  } else panicT = 0;

  if (digitalRead(BTN_RESET) == LOW) {
    fallState = NO_FALL; pkt.fallDetected = false; pkt.panicPressed = false; pkt.gasDetected = false;
    pkt.safetyStatus = 0; pkt.emergencyType = 0; currentAlert.active = false;
    digitalWrite(LED_RED, LOW); digitalWrite(BUZZER_PIN, LOW);
    Serial.println("[RESET] All cleared"); delay(300);
  }

  handleZone();
}
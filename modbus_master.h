#pragma once
#include <ModbusMaster.h>

// ============================================================
// Modbus RTU Master — Serial2 (GPIO4/5 RS485)
// Controls external Waveshare Modbus RTU Relay boards
// ============================================================
//
// Up to 4 external slave devices (8ch each = 32 external relays)
// FC05: Write Single Coil (relay ON/OFF)
// FC01: Read Coils (relay state readback)

const int MB_MAX_SLAVES = 4;

struct MbSlaveDevice {
  bool    enabled;
  uint8_t addr;       // Modbus slave address (1-247)
  char    label[16];  // display label (e.g. "Valve-A")
  uint8_t ch_count;   // number of relay channels (1-8, default 8)
};

struct ModbusMasterConfig {
  bool          enabled;
  int           baud_rate;
  int           poll_interval;  // status read interval (seconds, 0=disabled)
  MbSlaveDevice slaves[MB_MAX_SLAVES];
};

ModbusMasterConfig mbMaster = {false, 9600, 10, {}};

// Runtime
ModbusMaster mbNode;
uint8_t  mbExtRelayState[MB_MAX_SLAVES] = {0};  // cached coil state per slave
unsigned long mbLastPoll = 0;
bool     mbInitialized = false;

// ========== Forward declarations ==========
extern uint16_t modbusCalcCRC(const uint8_t* data, size_t len);

// ========== Config persistence ==========
void loadModbusMasterConfig() {
  File f = LittleFS.open("/modbus_master.json", "r");
  if (!f) return;
  JsonDocument doc;
  if (deserializeJson(doc, f)) { f.close(); return; }
  f.close();
  mbMaster.enabled       = doc["enabled"] | false;
  mbMaster.baud_rate     = doc["baud"]    | 9600;
  mbMaster.poll_interval = doc["poll"]    | 10;
  JsonArray arr = doc["slaves"].as<JsonArray>();
  for (int i = 0; i < MB_MAX_SLAVES; i++) {
    if (arr && i < (int)arr.size()) {
      JsonObject s = arr[i];
      mbMaster.slaves[i].enabled  = s["en"]   | false;
      mbMaster.slaves[i].addr     = s["addr"]  | (i + 1);
      mbMaster.slaves[i].ch_count = s["ch"]    | 8;
      strlcpy(mbMaster.slaves[i].label, s["label"] | "", sizeof(mbMaster.slaves[i].label));
    } else {
      mbMaster.slaves[i] = {false, (uint8_t)(i + 1), "", 8};
    }
  }
}

void saveModbusMasterConfig() {
  JsonDocument doc;
  doc["enabled"] = mbMaster.enabled;
  doc["baud"]    = mbMaster.baud_rate;
  doc["poll"]    = mbMaster.poll_interval;
  JsonArray arr = doc["slaves"].to<JsonArray>();
  for (int i = 0; i < MB_MAX_SLAVES; i++) {
    JsonObject s = arr.add<JsonObject>();
    s["en"]    = mbMaster.slaves[i].enabled;
    s["addr"]  = mbMaster.slaves[i].addr;
    s["ch"]    = mbMaster.slaves[i].ch_count;
    s["label"] = mbMaster.slaves[i].label;
  }
  File f = LittleFS.open("/modbus_master.json", "w");
  if (f) { serializeJson(doc, f); f.close(); }
}

// ========== Init ==========
void modbusMasterInit() {
  loadModbusMasterConfig();
  if (mbMaster.enabled) {
    if (mbMaster.baud_rate != RS485_DEFAULT_BAUD) {
      Serial2.end();
      Serial2.begin(mbMaster.baud_rate);
    }
    // Init with first enabled slave (re-configured per transaction)
    mbNode.begin(1, Serial2);
    mbInitialized = true;
    Serial.printf("[MODBUS-M] Master enabled baud=%d poll=%ds\n",
                  mbMaster.baud_rate, mbMaster.poll_interval);
  } else {
    Serial.println("[MODBUS-M] Master disabled");
  }
}

// ========== Relay control ==========
// Write single coil on external slave
bool mbWriteCoil(int slaveIdx, uint8_t ch, bool on) {
  if (!mbMaster.enabled || !mbInitialized) return false;
  if (slaveIdx < 0 || slaveIdx >= MB_MAX_SLAVES) return false;
  if (!mbMaster.slaves[slaveIdx].enabled) return false;
  if (ch >= mbMaster.slaves[slaveIdx].ch_count) return false;

  mbNode.begin(mbMaster.slaves[slaveIdx].addr, Serial2);
  uint8_t result = mbNode.writeSingleCoil(ch, on ? 0xFF00 : 0x0000);
  if (result == mbNode.ku8MBSuccess) {
    if (on) mbExtRelayState[slaveIdx] |=  (1 << ch);
    else    mbExtRelayState[slaveIdx] &= ~(1 << ch);
    Serial.printf("[MODBUS-M] Slave%d(addr=%d) CH%d %s\n",
                  slaveIdx + 1, mbMaster.slaves[slaveIdx].addr, ch + 1, on ? "ON" : "OFF");
    return true;
  }
  Serial.printf("[MODBUS-M] Slave%d FC05 FAIL err=%d\n", slaveIdx + 1, result);
  return false;
}

// Read coil status from external slave
bool mbReadCoils(int slaveIdx) {
  if (!mbMaster.enabled || !mbInitialized) return false;
  if (slaveIdx < 0 || slaveIdx >= MB_MAX_SLAVES) return false;
  if (!mbMaster.slaves[slaveIdx].enabled) return false;

  mbNode.begin(mbMaster.slaves[slaveIdx].addr, Serial2);
  uint8_t result = mbNode.readCoils(0x0000, mbMaster.slaves[slaveIdx].ch_count);
  if (result == mbNode.ku8MBSuccess) {
    mbExtRelayState[slaveIdx] = mbNode.getResponseBuffer(0) & 0xFF;
    return true;
  }
  Serial.printf("[MODBUS-M] Slave%d FC01 FAIL err=%d\n", slaveIdx + 1, result);
  return false;
}

// ========== Periodic poll ==========
void modbusMasterService(unsigned long now) {
  if (!mbMaster.enabled || !mbInitialized) return;
  if (mbMaster.poll_interval <= 0) return;
  if (now - mbLastPoll < (unsigned long)mbMaster.poll_interval * 1000UL) return;
  mbLastPoll = now;

  for (int i = 0; i < MB_MAX_SLAVES; i++) {
    if (!mbMaster.slaves[i].enabled) continue;
    mbReadCoils(i);
    yield();  // USB-NCM between slaves
  }
}

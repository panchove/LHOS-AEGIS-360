#pragma once

#ifndef __BLECONFIGLAYRZHUB_H__
#define __BLECONFIGLAYRZHUB_H__

#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/layrz_protocol/LinkLayrzHub.h>
#include <modules/startup/StartupLayrzHub.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

#define BLE_INSTANCE_ID 0

static std::string rxMsg;
static std::string rxPayload;

static int rxPayloadSize;
static int rxChunkCount;
static QueueHandle_t xQueueCommand = NULL;

void confiotOverBLE(void *pvParameters);

class BleConnectionCallbacks : public BLEServerCallbacks {
public:
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override;
  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo,
                    int reason) override;
  void onMTUChange(uint16_t MTU, NimBLEConnInfo &connInfo) override;
};

class WriteCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic,
               NimBLEConnInfo &connInfo) override;
};

class ReadCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic *pCharacteristic,
              NimBLEConnInfo &connInfo) override;
  void onSubscribe(NimBLECharacteristic *pCharacteristic,
                   NimBLEConnInfo &connInfo, uint16_t subValue) override;
};

#endif
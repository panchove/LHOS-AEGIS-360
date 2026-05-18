#pragma once
#ifndef __MODBUSLAYRZHUB_H__
#define __MODBUSLAYRZHUB_H__
#include "ModbusClientRTU.h"

#include <SPIFFS.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/gnss/GnssLayrzHub.h>
#include <modules/layrz_protocol/LinkLayrzHub.h> // Layrz protocol
#include <modules/settings/SettingsLayrzHub.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;

class ModbusLayrzHub {
public:
  static ModbusClientRTU modbusClient;
  static bool initModbus();

private:
  static void _handleModbusData(ModbusMessage response, uint32_t token);
  static void _handleModbusError(Error error, uint32_t token);
  static void _sendModbusRequest(uint32_t token, uint8_t serverId,
                                 uint8_t functionCode, uint16_t startAddress,
                                 uint16_t numRegisters);
  static std::string _decodeModbusMessage(uint8_t *data, size_t length,
                                          uint16_t serverId,
                                          uint8_t modbusAddress,
                                          uint8_t functionCode,
                                          uint16_t startAddress);
  static void _processModbusError(Error error);
  static void _modbusPollTask(void *pvParameters);
  static void _modbusPublishTask(void *pvParameters);
  static SpiRamJsonDocument *_jsonModbusMap;
  static bool _loadModbusMap();
};
extern bool dataReady; // Flag to indicate if data is ready
extern uint8_t modbusData[30];
extern uint8_t modbusAddresses[4]; // Array to hold Modbus addresses
extern uint8_t modbusDeviceIds[4]; // Array to hold Modbus devices

#endif // __MODBUSLAYRZHUB_H__
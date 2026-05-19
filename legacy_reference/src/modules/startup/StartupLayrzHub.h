#pragma once

#ifndef __STARTUPLAYRZHUB_H__
#define __STARTUPLAYRZHUB_H__

#include <modules/arducam/ArducamLayrzHub.h>
#include <modules/blackbox/BlackBoxLayrzHub.h>
#include <modules/ble_devices/BleLayrzHub.h>
#include <modules/confiot_ble/BleConfigLayrzHub.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/gpio/GpioLayrzHub.h>
#include <modules/j1939/J1939LayrzHub.h>
#include <modules/layrz_protocol/LinkLayrzHub.h>
#include <modules/messaging/MessageBusLayrzHub.h>
#include <modules/modbus/ModbusLayrzHub.h>
#include <modules/network/NetLayrzHub.h>
#include <modules/obd2/obd2LayrzHub.h>
#include <modules/rgbled/RgbLayrzHub.h>
#include <modules/sensors/SensorPublisherLayrzHub.h>
#include <modules/serial_comm/SerialLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>
#include <modules/zigbee/ZigbeeLayrzHub.h>

class StartupLayrzHub {
public:
  static void InitUsbCommunications();
  static void checkFactoryResetOnBoot();
  static void initDeviceSettings();
  static void initBlackBox();
  static void resetTime();
  static void initRgbLed(const char *greeting);
  static void initWatchdogTimer(int timeout);
  static bool createBinarySemaphore();
  static void startSystemHealthMonitor();
  static void startConfiotOverBLE();
  static void initA7670Module();
  static void initGnss();
  static void connectToNetwork(std::string netmode);
  static void checkForFirmwareUpdate();
  static void startNetworkKeepAlive();
  static void startTcpSocketListener();
  static bool initDataBuffers();
  static void initGpio();
  static void initBleSensors();
  static void initSensorsMessaging();
  static void initSerialCommunications();
  static void initCanBus();
  static void startModbus();
  static void initSPI();
  static void initArducamMega();
  static void startSynchroNtpTask();
  static void startLayrzProtocol();
  static void InitNimBLE();
  static void startArducamCyclicCaptures();
  static void initZigbeeCoordinator();

private:
  static bool _initMsgBuffer();
  static bool _initSettingsBuffer();
  static bool _initUartIoBuffer();
  static bool _initRs485Buffer();
  static bool _initRs232_1Buffer();
  static bool _initRs232_2Buffer();
  static bool _initExtIoBuffer();
  static bool _initCanbusBuffer();
  static bool _initObd2DtcBuffer();
  static bool _initMediaBuffer();
  static bool _initModbusBuffer();
};

#endif // __STARTUPLAYRZHUB_H__

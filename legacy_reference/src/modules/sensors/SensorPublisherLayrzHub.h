#pragma once
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/gnss/GnssLayrzHub.h>
#include <modules/gpio/GpioLayrzHub.h>
#include <modules/messaging/MessageBusLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>

class SensorPublisherLayrzHub {
public:
  static void updateSensors(void *pvParameters);
  static void sendSensorData(void *pvParameters);
  static void pingTask(void *pvParameters);
  static void buildSensorPayload(bool gpioEn, const std::string &position);
};

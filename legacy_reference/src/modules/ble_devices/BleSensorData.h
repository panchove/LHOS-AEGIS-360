// Central shared BLE sensor data structure.
// Add any common fields here so all device decoders can reuse it without
// redefining the struct in multiple headers (avoids ODR redefinition errors).
#pragma once
#ifndef BLE_SENSOR_DATA_LAYRZHUB_H
#define BLE_SENSOR_DATA_LAYRZHUB_H

#include <string>

struct bleSensorData {
  std::string id;      // Lowercased identifier / UUID / key
  std::string hexData; // Raw hex payload associated with the id
};

#endif // BLE_SENSOR_DATA_LAYRZHUB_H

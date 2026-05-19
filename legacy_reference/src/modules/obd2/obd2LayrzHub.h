#pragma once

#ifndef __OBD2LAYRZHUB_H__
#define __OBD2LAYRZHUB_H__

#include "OBD2.h" // OBD2 stack

#include <modules/global_objects/GlobalObjectsLayrzHub.h> // Global objects
#include <modules/gnss/GnssLayrzHub.h>                    // GNSS module
#include <modules/layrz_protocol/LinkLayrzHub.h>          // Layrz protocol
#include <modules/obd2/obd2_decoder.h>

class Obd2LayrzHub {
public:
  static uint32_t pid;
  static bool initObd2();
  static void obd2MonitorTask(void *pvParameters);
  static int getObd2Dtcs();
  static uint8_t fillSupportedPidsList();
};

#endif // __OBD2LAYRZHUB_H__
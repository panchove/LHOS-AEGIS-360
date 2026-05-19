#pragma once

#ifndef __J1939LAYRZHUB_H__
#define __J1939LAYRZHUB_H__

#include "ARD1939ESP32S3.h" // J1939 stack

#include <modules/global_objects/GlobalObjectsLayrzHub.h> // Global objects
#include <modules/gnss/GnssLayrzHub.h>                    // GNSS module
#include <modules/j1939/j1939_decoder.h>
#include <modules/layrz_protocol/LinkLayrzHub.h> // Layrz protocol

class J1939LayrzHub {
public:
  static ARD1939ESP32S3 j1939;
  static J1939Decoder decoder;
  static byte msgType;
  static uint32_t pgn;
  static byte msgData[MAX_TRANSPORT_LENGTH];
  static int msgLen;
  static byte destAddr;
  static byte srcAddr;
  static byte priority;
  static bool initJ1939();
  static void j1939MonitorTask(void *pvParameters);
};

#endif // __J1939LAYRZHUB_H__
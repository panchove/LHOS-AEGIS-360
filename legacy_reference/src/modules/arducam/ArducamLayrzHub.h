#pragma once
#ifndef __ARDUCAMLAYRZHUB_H__
#define __ARDUCAMLAYRZHUB_H__
#include <Arducam_Mega.h>
#include <Arduino.h>
#include <SPI.h>
#include <base64.h> // ArduinoBase64 library
#include <modules/definitions/DefinitionsLayrzHub.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/layrz_protocol/LinkLayrzHub.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class ArducamLayrzHub {
public:
  static bool initArducamMega();
  static bool takePicture();
  static void cyclicArducamTask(void *pvParameters);
};

extern Arducam_Mega HubCam;

#endif // __ARDUCAMLAYRZHUB_H__
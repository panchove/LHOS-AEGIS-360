#pragma once

#ifndef __FOTALAYRZHUB_H__
#define __FOTALAYRZHUB_H__

#include <ArduinoOTA.h>
#include <FS.h>
#include <SD.h>
#include <Update.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/rgbled/RgbLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>

bool fotaUpgrade(const char *url);
void checkFirmware(void *pvParameters);

#endif
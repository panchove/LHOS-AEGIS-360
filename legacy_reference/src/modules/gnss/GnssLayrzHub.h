#pragma once

#ifndef __GNSSLAYRZHUB_H__
#define __GNSSLAYRZHUB_H__

#include <Arduino.h>
#include <TimeLib.h>
#include <modules/network/NetLayrzHub.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class GNSS {
public:
  static void initGNSS(int mode);
  static std::string getPosition();

private:
  static int findCharIndex(const char *str, char ch);
};

#endif
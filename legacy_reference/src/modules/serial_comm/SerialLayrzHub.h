#pragma once

#ifndef __SERIALLAYRZHUB_H__
#define __SERIALLAYRZHUB_H__

#include <iomanip>
#include <iostream>
#include <modules/arducam/ArducamLayrzHub.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/layrz_protocol/LinkLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>
#include <sstream>
#include <string>

class SerialMonitor {
public:
  static void init();
  static void monitorPorts();
  static bool resetHpc168(uint16_t frontAddr, uint16_t backAddr,
                          bool backAvailable);
  static int setExternalOutput(uint8_t pin, uint8_t value, uint8_t mode);
  // static int setExternalDO(uint8_t pin, uint8_t value);
  static bool rebootIOExtender();
  static void serialMonitorTask(void *pvParameters);
  static std::vector<uint8_t> hexStringToBytes(const std::string &hex);
  static std::string stringToHexString(const std::string &input);
  static void setDN23E08DigitalOutput(uint8_t rs485Addr, uint8_t doIndex,
                                      uint8_t doValue, uint8_t doDuration);
  static void setDN23E08DigitalOutputs(uint8_t startAddr, uint8_t endAddr,
                                       byte doByte, uint8_t *doDuration);
  static void rebootDN23E08(uint8_t startAddr, uint8_t endAddr);
  static void resetDN23E08TriggerCounter(uint8_t startAddr, uint8_t endAddr);

private:
  struct commandResponse {
    std::string response_frame;
    bool response_valid;
    bool door_closed;
    uint32_t passengers_in_value;
    uint32_t passengers_out_value;
  }; // Struct to hold the response from the HPC168

  static void getTransData(HardwareSerial &serialPort, const char *portName,
                           uint16_t size, std::string startMarker,
                           std::string endMarker, int bufferSize);
  static void getTransData(SoftwareSerial &serialPort, const char *portName,
                           uint16_t size, std::string startMarker,
                           std::string endMarker, int bufferSize);
  static void getA01nyub(SoftwareSerial &serialPort);
  static void getIoExtenderData(
    SoftwareSerial &serialPort); // Function to get data from the IO extender
  static void getHpc168Data(uint16_t frontAddr, uint16_t backAddr,
                            bool backAvailable);
  static std::string
  buildDN23E08Cmd(byte addr, byte cmd, int length, byte doByte = 0x00,
                  uint8_t *doDuration =
                    NULL); // Function to build the command frame for DN23E08
  static std::string buildDN23E08SetDoCmd(
    uint8_t rs485Addr, uint8_t doIndex, uint8_t doValue,
    uint8_t doDuration); // Function to build the command frame for DN23E08
  static std::vector<std::string> extractFrames(
    const std::string &buffer); // Function to extract frames from the buffer
  static void getDN23E08Data(uint8_t startAddr, uint8_t endAddr);

  static int findBytes(const std::string &buffer,
                       const std::vector<uint8_t> &pattern, int start = 0);
  static std::string send_hpc_command(
    Stream &serial_port,
    const std::string &command_frame); // Function to send the command and
                                       // receive the response over RS485
  static void rs485_set_mode(bool write_mode);
  static std::string build_command_frame(
    uint16_t id, uint8_t command, uint8_t length,
    const std::string &data); // Function to build the command frame for HPC168
  static std::string calculate_checksum(
    const std::string &hex_chain); // Function to calculate checksum of HPC168
                                   // command and response frame
  static std::string int_to_hex(
    uint64_t value,
    size_t width); // Function to convert int to hex string with leading zeros
  static commandResponse
  parse_response(const std::string
                   &response); // Function to parse the response of the HPC168
  // functions used by ri505_ticket RS232 mode
  // Helpers:
  static std::vector<uint8_t> hexToBytes(const std::string &hex);
  static std::string trim(const std::string &s);
  static std::string collapseSpaces(const std::string &s);
  static std::string jsonEscape(const std::string &s);
  static std::string toUpperCopy(const std::string &s);
  static std::string normalizeNumber(const std::string &s);
  // Esc/Pos stripping
  static std::vector<uint8_t> stripEscPos(const std::vector<uint8_t> &in);
  static std::string bytesToCleanText(const std::vector<uint8_t> &data);
  static std::string collapseSpacesPerLinePreserveNL(const std::string &s);
  static std::vector<std::string>
  splitAndNormalizeLines(const std::string &text);
  // Pattern match helpers
  static bool looksLikeDate_ddmmyy(const std::string &s);
  static bool looksLikeTime_hhmmss(const std::string &s);
  static std::string parseNumberAtEnd(const std::string &line);
  static std::string parseFieldLine(const std::string &line,
                                    const std::string &keyUpper);
  static std::string parseTokenField(const std::string &line,
                                     const std::string &keyUpper);
  static std::string parseTotalSmart(const std::string &line);
  // Main parser
  static std::string calculate_json(const std::string &raw_ticket_hex);
};

struct DN23E08Frame {
  uint8_t address;
  uint8_t command;
  uint8_t digitalInputs[8];
  uint8_t digitalOutputs[8];
  uint16_t triggerCount;
  uint16_t currents[4];
  uint16_t voltages[4];
  bool validCRC;

  static bool checkCRC(const std::string &msg);
  static DN23E08Frame parse(const std::string &msg);
};

extern SemaphoreHandle_t rs485Semaphore; // Declare the semaphore handle

#endif

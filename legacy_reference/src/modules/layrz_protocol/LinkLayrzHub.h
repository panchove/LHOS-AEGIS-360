#pragma once

#ifndef __LINKLAYRZHUB_H__
#define __LINKLAYRZHUB_H__

#include <modules/arducam/ArducamLayrzHub.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/gnss/GnssLayrzHub.h>
#include <modules/messaging/ClientSelectorLayrzHub.h>
#include <modules/obd2/obd2LayrzHub.h>
#include <modules/rgbled/RgbLayrzHub.h>
#include <modules/sensors/SensorPublisherLayrzHub.h>
#include <modules/serial_comm/SerialLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>

#define BLE_DELAY_PER_CHUNK 150 // delay between chunks (in milliseconds)

struct setKey {
  bool isValid;
  std::string key;
  std::string value;
};

enum cmdSource {
  BLE,
  NET,
  UART,
};

class LayrzProtocol {
public:
  static void sendBleConfiotMsg(const std::string &data);
  // static serverResponse sendSocketData(char *data);
  // static serverResponse postData (const char* server, int port, const char*
  // path, char* data, const char* httpAuth, int trials); static serverResponse
  // getCmdQueue(const char* server, int port, const char* path, const char*
  // httpAuth, int trials); static void sendCmdResponse(const char * cmd_id,
  // const char * response, int cmdSource);
  static setKey parseKeyValue(std::string input);
  static void executeCmd(const char *cmd, const char *cmd_id, const char *args,
                         const char *crc, int cmdSource);
  static void cmdWrapper(std::string cmd, int cmdSource);
  static void getCommands(void *pvParameters);

private:
  // Command handlers
  static void reboot_Handler(const char *cmd_id, int cmdSource);
  static bool get_config_Handler(int cmdSource);
  static void set_config_Handler(const char *cmd_id, const char *args,
                                 int cmdSource);
  static bool get_info_Handler(int cmdSource);
  static void get_msg_Handler(const char *cmd_id, int cmdSource);
  static void set_do_Handler(const char *cmd_id, const char *args,
                             int cmdSource);
  static void set_pwm_output_Handler(const char *cmd_id, const char *args,
                                     int cmdSource);
  static void set_serial_Handler(const char *cmd_id, const char *args,
                                 int cmdSource);
  static void set_rgb_Handler(const char *cmd_id, const char *args,
                              int cmdSource);
  static bool ping_Handler(const char *cmd_id, int cmdSource);
  static void start_fota_Handler(const char *cmd_id, int cmdSource);
  static void factory_reset_Handler(const char *cmd_id, int cmdSource);
  static void hpc168_reset_Handler(const char *cmd_id, int cmdSource);
  static void set_ext_do_Handler(const char *cmd_id, const char *args,
                                 int cmdSource);
  static void set_ext_ao_Handler(const char *cmd_id, const char *args,
                                 int cmdSource);
  static void ext_io_reboot_Handler(const char *cmd_id, int cmdSource);
  static void get_obd2_dtcs_Handler(const char *cmd_id, int cmdSource);
  static void set_dn23e08_do_Handler(const char *cmd_id, const char *args,
                                     int cmdSource);
  static void set_dn23e08_multi_do_Handler(const char *cmd_id, const char *args,
                                           int cmdSource);
  static void reset_dn23e08_do_Handler(const char *cmd_id, const char *args,
                                       int cmdSource);
  static void dn23e08_reboot_Handler(const char *cmd_id, const char *args,
                                     int cmdSource);
  static void reset_dn23e08_counter_Handler(const char *cmd_id,
                                            const char *args, int cmdSource);
  static void take_photo_Handler(const char *cmd_id, int cmdSource);
  static void delete_blackbox_Handler(const char *cmd_id, int cmdSource);
  static void delete_history_Handler(const char *cmd_id, int cmdSource);
  static void delete_images_Handler(const char *cmd_id, int cmdSource);
  static void format_sd_Handler(const char *cmd_id, int cmdSource);

  // Utility functions
  static void sendCommandAck(const char *cmd_id, const char *msg,
                             int cmdSource);
};

#endif
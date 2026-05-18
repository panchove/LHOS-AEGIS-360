#include <cstring> // For strcmp
#include <modules/settings/SettingsLayrzHub.h>

const deviceSettings::settingsKeyList deviceSettings::settingsKeyListData[] = {
  {"sys_dev_id", "0", settingsKeyList::INT64},
  {"sys_dev_name", "LAYRZ.HUB", settingsKeyList::STRING},
#if defined(LAYRZ_HUB1_BUILD)
  {"sys_dev_hw_id", "1", settingsKeyList::INT16},
#elif defined(LAYRZ_HUB2_BUILD)
  {"sys_dev_hw_id", "2", settingsKeyList::INT16},
#elif defined(LAYRZ_HUB25_BUILD)
  {"sys_dev_hw_id", "38", settingsKeyList::INT16},
#endif
  {"sys_dev_md_id", "0", settingsKeyList::INT64},
  {"sys_cmd_per", "10", settingsKeyList::INT},
  {"sys_debug_en", "true", settingsKeyList::BOOL},
  {"sys_ntp_period", "2", settingsKeyList::INT},
  {"sys_ntpserver1", "pool.ntp.org", settingsKeyList::STRING},
  {"sys_ntpserver2", "time.nist.gov", settingsKeyList::STRING},
  {"sys_ble_timeout", "120", settingsKeyList::INT},
  {"sys_hist_en", "true", settingsKeyList::BOOL},
  {"net_en", "false", settingsKeyList::BOOL},
  {"net_mode", "wifi", settingsKeyList::STRING},
  {"net_server", "link.layrz.network", settingsKeyList::STRING},
  {"net_protocol", "0", settingsKeyList::INT}, // 0: TCP sockets, 1: HTTPS
  {"wifi_ssid", "hubone", settingsKeyList::STRING},
  {"wifi_pass", "hubone1234", settingsKeyList::STRING},
  {"gprs_apn", "", settingsKeyList::STRING},
  {"gprs_apn_user", "", settingsKeyList::STRING},
  {"gprs_apn_pass", "", settingsKeyList::STRING},
  {"gprs_pin", "0000", settingsKeyList::STRING},
  {"fota_en", "false", settingsKeyList::BOOL},
  {"fota_force", "false", settingsKeyList::BOOL},
#if defined(LAYRZ_HUB1_BUILD)
  {"fota_fw_id", "layrz.hub.one12", settingsKeyList::STRING},
#elif defined(LAYRZ_HUB2_BUILD)
  {"fota_fw_id", "layrz.hub.one2", settingsKeyList::STRING},
#elif defined(LAYRZ_HUB25_BUILD)
  {"fota_fw_id", "layrz.hub.one25", settingsKeyList::STRING},
#endif
  {"fota_fw_branch", "0", settingsKeyList::INT},
  {"fota_fw_ts", "0", settingsKeyList::INT},
  {"gnss_en", "false", settingsKeyList::BOOL},
  {"gnss_mode", "2", settingsKeyList::INT},
  {"gnss_use_static", "false", settingsKeyList::BOOL},
  {"gnss_static_lat", "0.0000000", settingsKeyList::FLOAT},
  {"gnss_static_lon", "0.0000000", settingsKeyList::FLOAT},
  {"gnss_static_alt", "0", settingsKeyList::INT},
#if defined(LAYRZ_HUB1_BUILD) || defined(LAYRZ_HUB2_BUILD)
  {"gpio_1_mode", "0", settingsKeyList::INT},
  {"gpio_2_mode", "0", settingsKeyList::INT},
  {"gpio_3_mode", "0", settingsKeyList::INT},
  {"gpio_4_mode", "0", settingsKeyList::INT},
  {"gpio_5_mode", "0", settingsKeyList::INT},
  {"gpio_6_mode", "0", settingsKeyList::INT},
  {"gpio_7_mode", "0", settingsKeyList::INT},
  {"gpio_14_mode", "0", settingsKeyList::INT},
  {"gpio_19_mode", "0", settingsKeyList::INT},
  {"gpio_20_mode", "0", settingsKeyList::INT},
  {"gpio_41_mode", "0", settingsKeyList::INT},
  {"gpio_42_mode", "0", settingsKeyList::INT},
  {"gpio_45_mode", "0", settingsKeyList::INT},
  {"gpio_46_mode", "0", settingsKeyList::INT},
  {"gpio_47_mode", "0", settingsKeyList::INT},
#elif defined(LAYRZ_HUB25_BUILD)
  {"di1_mode", "0",
   settingsKeyList::INT}, // GPIO4: 0: digital input, 1: digital input
                          // pull-down, 2: digital input pull-up
  {"di2_mode", "0",
   settingsKeyList::INT}, // GPIO5: 0: digital input, 1: digital input
                          // pull-down, 2: digital input pull-up
  {"di3_mode", "0",
   settingsKeyList::INT}, // GPIO6: 0: digital input, 1: digital input
                          // pull-down, 2: digital input pull-up
  {"di4_mode", "0",
   settingsKeyList::INT}, // GPIO42: 0: digital input, 1: digital input
                          // pull-down, 2: digital input pull-up
  {"ai1_di5_mode", "0",
   settingsKeyList::INT}, // GPIO7: 0: digital input, 1: digital input
                          // pull-down, 2: digital input pull-up, 4: analog
                          // input
  {"ai2_di6_mode", "0",
   settingsKeyList::INT}, // GPIO8: 0: digital input, 1: digital input
                          // pull-down, 2: digital input pull-up, 4: analog
                          // input
  {"gpio_19_mode", "0", settingsKeyList::INT},
  {"gpio_20_mode", "0", settingsKeyList::INT},
#endif
  {"ble_0_model", "", settingsKeyList::STRING},
  {"ble_0_address", "", settingsKeyList::STRING},
  {"ble_1_model", "", settingsKeyList::STRING},
  {"ble_1_address", "", settingsKeyList::STRING},
  {"ble_2_model", "", settingsKeyList::STRING},
  {"ble_2_address", "", settingsKeyList::STRING},
  {"ble_3_model", "", settingsKeyList::STRING},
  {"ble_3_address", "", settingsKeyList::STRING},
  {"ble_4_model", "", settingsKeyList::STRING},
  {"ble_4_address", "", settingsKeyList::STRING},
  {"ble_5_model", "", settingsKeyList::STRING},
  {"ble_5_address", "", settingsKeyList::STRING},
  {"ble_6_model", "", settingsKeyList::STRING},
  {"ble_6_address", "", settingsKeyList::STRING},
  {"ble_7_model", "", settingsKeyList::STRING},
  {"ble_7_address", "", settingsKeyList::STRING},
  {"ble_8_model", "", settingsKeyList::STRING},
  {"ble_8_address", "", settingsKeyList::STRING},
  {"ble_9_model", "", settingsKeyList::STRING},
  {"ble_9_address", "", settingsKeyList::STRING},
  {"ble_10_model", "", settingsKeyList::STRING},
  {"ble_10_address", "", settingsKeyList::STRING},
  {"ble_11_model", "", settingsKeyList::STRING},
  {"ble_11_address", "", settingsKeyList::STRING},
  {"ble_12_model", "", settingsKeyList::STRING},
  {"ble_12_address", "", settingsKeyList::STRING},
  {"ble_13_model", "", settingsKeyList::STRING},
  {"ble_13_address", "", settingsKeyList::STRING},
  {"ble_14_model", "", settingsKeyList::STRING},
  {"ble_14_address", "", settingsKeyList::STRING},
  {"ble_15_model", "", settingsKeyList::STRING},
  {"ble_15_address", "", settingsKeyList::STRING},
  {"ble_16_model", "", settingsKeyList::STRING},
  {"ble_16_address", "", settingsKeyList::STRING},
  {"ble_17_model", "", settingsKeyList::STRING},
  {"ble_17_address", "", settingsKeyList::STRING},
  {"ble_18_model", "", settingsKeyList::STRING},
  {"ble_18_address", "", settingsKeyList::STRING},
  {"ble_19_model", "", settingsKeyList::STRING},
  {"ble_19_address", "", settingsKeyList::STRING},
  {"ble_20_model", "", settingsKeyList::STRING},
  {"ble_20_address", "", settingsKeyList::STRING},
  {"ble_21_model", "", settingsKeyList::STRING},
  {"ble_21_address", "", settingsKeyList::STRING},
  {"ble_22_model", "", settingsKeyList::STRING},
  {"ble_22_address", "", settingsKeyList::STRING},
  {"ble_23_model", "", settingsKeyList::STRING},
  {"ble_23_address", "", settingsKeyList::STRING},
  {"ble_24_model", "", settingsKeyList::STRING},
  {"ble_24_address", "", settingsKeyList::STRING},
  {"ble_25_model", "", settingsKeyList::STRING},
  {"ble_25_address", "", settingsKeyList::STRING},
  {"ble_26_model", "", settingsKeyList::STRING},
  {"ble_26_address", "", settingsKeyList::STRING},
  {"ble_27_model", "", settingsKeyList::STRING},
  {"ble_27_address", "", settingsKeyList::STRING},
  {"ble_28_model", "", settingsKeyList::STRING},
  {"ble_28_address", "", settingsKeyList::STRING},
  {"ble_29_model", "", settingsKeyList::STRING},
  {"ble_29_address", "", settingsKeyList::STRING},
  {"ble_30_model", "", settingsKeyList::STRING},
  {"ble_30_address", "", settingsKeyList::STRING},
  {"ble_31_model", "", settingsKeyList::STRING},
  {"ble_31_address", "", settingsKeyList::STRING},
  {"ble_32_model", "", settingsKeyList::STRING},
  {"ble_32_address", "", settingsKeyList::STRING},
  {"ble_33_model", "", settingsKeyList::STRING},
  {"ble_33_address", "", settingsKeyList::STRING},
  {"ble_34_model", "", settingsKeyList::STRING},
  {"ble_34_address", "", settingsKeyList::STRING},
  {"ble_35_model", "", settingsKeyList::STRING},
  {"ble_35_address", "", settingsKeyList::STRING},
  {"ble_36_model", "", settingsKeyList::STRING},
  {"ble_36_address", "", settingsKeyList::STRING},
  {"ble_37_model", "", settingsKeyList::STRING},
  {"ble_37_address", "", settingsKeyList::STRING},
  {"ble_38_model", "", settingsKeyList::STRING},
  {"ble_38_address", "", settingsKeyList::STRING},
  {"ble_39_model", "", settingsKeyList::STRING},
  {"ble_39_address", "", settingsKeyList::STRING},
  {"ble_40_model", "", settingsKeyList::STRING},
  {"ble_40_address", "", settingsKeyList::STRING},
  {"ble_41_model", "", settingsKeyList::STRING},
  {"ble_41_address", "", settingsKeyList::STRING},
  {"ble_42_model", "", settingsKeyList::STRING},
  {"ble_42_address", "", settingsKeyList::STRING},
  {"ble_43_model", "", settingsKeyList::STRING},
  {"ble_43_address", "", settingsKeyList::STRING},
  {"ble_44_model", "", settingsKeyList::STRING},
  {"ble_44_address", "", settingsKeyList::STRING},
  {"ble_45_model", "", settingsKeyList::STRING},
  {"ble_45_address", "", settingsKeyList::STRING},
  {"ble_46_model", "", settingsKeyList::STRING},
  {"ble_46_address", "", settingsKeyList::STRING},
  {"ble_47_model", "", settingsKeyList::STRING},
  {"ble_47_address", "", settingsKeyList::STRING},
  {"ble_48_model", "", settingsKeyList::STRING},
  {"ble_48_address", "", settingsKeyList::STRING},
  {"ble_49_model", "", settingsKeyList::STRING},
  {"ble_49_address", "", settingsKeyList::STRING},
  {"uart_en", "false", settingsKeyList::BOOL},
  {"uart_rx_pin", "1", settingsKeyList::INT},
  {"uart_tx_pin", "2", settingsKeyList::INT},
  {"uart_mode", "transparent", settingsKeyList::STRING},
  {"uart_buffer", "256", settingsKeyList::INT},
  {"uart_start", "", settingsKeyList::STRING},
  {"uart_end", "", settingsKeyList::STRING},
  {"uart_baud", "9600", settingsKeyList::INT},
  {"uart_bits", "8", settingsKeyList::INT},
  {"uart_par", "none", settingsKeyList::STRING},
  {"rs485_en", "true", settingsKeyList::BOOL},
  {"rs485_delay", "15", settingsKeyList::INT},
  {"rs485_mode", "transparent", settingsKeyList::STRING},
  {"rs485_buffer", "256", settingsKeyList::INT},
  {"rs485_start", "", settingsKeyList::STRING},
  {"rs485_end", "", settingsKeyList::STRING},
  {"rs485_baud", "9600", settingsKeyList::INT},
  {"rs485_bits", "8", settingsKeyList::INT},
  {"rs485_par", "none", settingsKeyList::STRING},
  {"rs232_1_en", "true", settingsKeyList::BOOL},
  {"rs232_1_mode", "transparent", settingsKeyList::STRING},
  {"rs232_1_buffer", "256", settingsKeyList::INT},
  {"rs232_1_start", "", settingsKeyList::STRING},
  {"rs232_1_end", "", settingsKeyList::STRING},
  {"rs232_1_baud", "9600", settingsKeyList::INT},
  {"rs232_1_bits", "8", settingsKeyList::INT},
  {"rs232_1_par", "none", settingsKeyList::STRING},
  {"rs232_2_en", "true", settingsKeyList::BOOL},
  {"rs232_2_mode", "transparent", settingsKeyList::STRING},
  {"rs232_2_buffer", "256", settingsKeyList::INT},
  {"rs232_2_start", "", settingsKeyList::STRING},
  {"rs232_2_end", "", settingsKeyList::STRING},
  {"rs232_2_baud", "9600", settingsKeyList::INT},
  {"rs232_2_bits", "8", settingsKeyList::INT},
  {"rs232_2_par", "none", settingsKeyList::STRING},
  {"ri_api_url", "", settingsKeyList::STRING},
  {"ri_api_token", "", settingsKeyList::STRING},
  {"ri_api_ident", "", settingsKeyList::STRING},
  {"ri_api_permit", "", settingsKeyList::STRING},
  {"acc_en", "false", settingsKeyList::BOOL},
  {"acc_mode", "0", settingsKeyList::INT},
  {"acc_grav_filter", "false", settingsKeyList::BOOL},
  {"acc_grav_thr", "9810", settingsKeyList::INT},
  {"rgb_en", "true", settingsKeyList::BOOL},
  {"rgb_count", "1", settingsKeyList::INT},
  {"rgb_brightness", "2", settingsKeyList::INT},
  {"data_upd_per", "30", settingsKeyList::INT},
  {"data_pub_per", "60", settingsKeyList::INT},
  {"data_ble_en", "true", settingsKeyList::BOOL},
  {"data_ble_dec_en", "false", settingsKeyList::BOOL},
  {"data_ble_win", "10", settingsKeyList::INT},
  {"data_serial_en", "true", settingsKeyList::BOOL},
  {"data_gpio_en", "true", settingsKeyList::BOOL},
  {"onewire_en", "false", settingsKeyList::BOOL},
  {"onewire_pin", "14", settingsKeyList::INT},
  {"onewire_0_model", "", settingsKeyList::STRING},
  {"onewire_0_addr", "", settingsKeyList::STRING},
  {"onewire_1_model", "", settingsKeyList::STRING},
  {"onewire_1_addr", "", settingsKeyList::STRING},
  {"onewire_2_model", "", settingsKeyList::STRING},
  {"onewire_2_addr", "", settingsKeyList::STRING},
  {"onewire_3_model", "", settingsKeyList::STRING},
  {"onewire_3_addr", "", settingsKeyList::STRING},
  {"onewire_4_model", "", settingsKeyList::STRING},
  {"onewire_4_addr", "", settingsKeyList::STRING},
  {"onewire_5_model", "", settingsKeyList::STRING},
  {"onewire_5_addr", "", settingsKeyList::STRING},
  {"hpc_front_addr", "1", settingsKeyList::INT},
  {"hpc_back_addr", "2", settingsKeyList::INT},
  {"hpc_back_en", "false", settingsKeyList::BOOL},
  {"can_en", "false", settingsKeyList::BOOL},
  {"can_mode", "2", settingsKeyList::INT},
  {"can_decode_en", "true", settingsKeyList::BOOL},
  {"can_j1939_filt", "0", settingsKeyList::INT},
  {"can_obd2_filt", "0", settingsKeyList::INT},
  {"can_dtc_en", "false", settingsKeyList::BOOL},
  {"can_j1939_dtc_f", "0", settingsKeyList::INT},
  {"can_obd2_dtc_f", "0", settingsKeyList::INT},
  {"can_poll_win", "2", settingsKeyList::INT},
  {"can_pub_per", "2", settingsKeyList::INT},
  {"can_nan_filter", "true", settingsKeyList::BOOL},
  {"dn23e08_s_addr", "1", settingsKeyList::INT},
  {"dn23e08_e_addr", "1", settingsKeyList::INT},
  {"acam_en", "false", settingsKeyList::BOOL},
  {"acam_name", "arducam", settingsKeyList::STRING},
  {"acam_model", "1", settingsKeyList::INT}, // 0: 3MP, 1: 5MP
  {"acam_res", "1",
   settingsKeyList::INT}, // 0: 160x120, 1: 320x240, 2: 640x480, 3: 800x600
  {"acam_wb_mode", "0",
   settingsKeyList::INT}, // 0: auto, 1: sunny, 2: office, 3: cloudy, 4: home
  {"acam_color_fx", "0",
   settingsKeyList::INT}, // 0: none, 1: blueish, 2: redish, 3: BW, 4: sepia, 5:
                          // negative, 6: grass green, 7: over exposure, 8:
                          // solarize
  {"acam_trig_src", "0",
   settingsKeyList::INT}, // 0: serial command, GPIOS 1,2,3,4,5,6,7,14
  {"acam_trig_cnt", "1", settingsKeyList::INT}, // number of captures at trigger
  {"acam_trig_int", "10", settingsKeyList::INT}, // interval in seconds
  {"acam_cyclic_en", "false", settingsKeyList::BOOL},
  {"acam_cyclic_int", "60", settingsKeyList::INT},
  {"acam_storage_en", "true", settingsKeyList::BOOL},
  {"modbus_addr1", "1", settingsKeyList::INT},
  {"modbus_addr2", "2", settingsKeyList::INT},
  {"modbus_addr3", "3", settingsKeyList::INT},
  {"modbus_addr4", "4", settingsKeyList::INT},
  {"modbus_dev1", "1",
   settingsKeyList::INT}, // 0: none, 1: wsfg06, 2: powerCommand2_3
  {"modbus_dev2", "0", settingsKeyList::INT},
  {"modbus_dev3", "0", settingsKeyList::INT},
  {"modbus_dev4", "0", settingsKeyList::INT},
  {"modbus_poll_int", "30",
   settingsKeyList::INT}, // Polling interval in seconds
  {"modbus_dec_en", "true", settingsKeyList::BOOL},
  {"zigbee_en", "false", settingsKeyList::BOOL},
  {"zigbee_local_en", "false", settingsKeyList::BOOL},
  {"zigbee_hub_url", "zigbee.layrz.network", settingsKeyList::STRING},
  {"zigbee_site_id", "", settingsKeyList::STRING},
  {"zigbee_token", "", settingsKeyList::STRING},
};

const int deviceSettings::settingsKeyListSize =
  sizeof(settingsKeyListData) / sizeof(settingsKeyListData[0]);

void deviceSettings::initDeviceSettings() {
  // Initialize the unified SPIFFS storage system
  if (!UnifiedSettingsStorage::init()) {
    debugPrint("Failed to initialize unified settings storage\n");
    return;
  }

  // Migrate critical network settings from legacy NVS if needed
  if (!UnifiedSettingsStorage::migrateNetworkSettingsFromNVS()) {
    debugPrint("Warning: Network settings migration from NVS failed\n");
  }

  String factoryDevName = getDeviceName().c_str();
  int i = 2;
  hubSettings.sys_dev_id = UnifiedSettingsStorage::getInt64(
    "sys_dev_id", atoll(settingsKeyListData[0].factoryValue));
  hubSettings.sys_dev_name =
    UnifiedSettingsStorage::getString("sys_dev_name", factoryDevName).c_str();
  hubSettings.sys_dev_hw_id = UnifiedSettingsStorage::getInt(
    "sys_dev_hw_id", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.sys_dev_md_id = UnifiedSettingsStorage::getInt64(
    "sys_dev_md_id", atoll(settingsKeyListData[i++].factoryValue));
  hubSettings.sys_cmd_per = UnifiedSettingsStorage::getInt(
    "sys_cmd_per", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.sys_debug_en = UnifiedSettingsStorage::getBool(
    "sys_debug_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.sys_ntp_period = UnifiedSettingsStorage::getInt(
    "sys_ntp_period", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.sys_ntpserver1 =
    UnifiedSettingsStorage::getString("sys_ntpserver1",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.sys_ntpserver2 =
    UnifiedSettingsStorage::getString("sys_ntpserver2",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.sys_ble_timeout = UnifiedSettingsStorage::getInt(
    "sys_ble_timeout", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.sys_hist_en = UnifiedSettingsStorage::getBool(
    "sys_hist_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.net_en = UnifiedSettingsStorage::getBool(
    "net_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.net_mode = UnifiedSettingsStorage::getString(
                           "net_mode", settingsKeyListData[i++].factoryValue)
                           .c_str();
  hubSettings.net_server =
    UnifiedSettingsStorage::getString("net_server",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.net_protocol = UnifiedSettingsStorage::getInt(
    "net_protocol", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.wifi_ssid = UnifiedSettingsStorage::getString(
                            "wifi_ssid", settingsKeyListData[i++].factoryValue)
                            .c_str();
  hubSettings.wifi_pass = UnifiedSettingsStorage::getString(
                            "wifi_pass", settingsKeyListData[i++].factoryValue)
                            .c_str();
  hubSettings.gprs_apn = UnifiedSettingsStorage::getString(
                           "gprs_apn", settingsKeyListData[i++].factoryValue)
                           .c_str();
  hubSettings.gprs_apn_user =
    UnifiedSettingsStorage::getString("gprs_apn_user",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.gprs_apn_pass =
    UnifiedSettingsStorage::getString("gprs_apn_pass",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.gprs_pin = UnifiedSettingsStorage::getString(
                           "gprs_pin", settingsKeyListData[i++].factoryValue)
                           .c_str();
  hubSettings.fota_en = UnifiedSettingsStorage::getBool(
    "fota_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.fota_force = UnifiedSettingsStorage::getBool(
    "fota_force", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.fota_fw_id =
    UnifiedSettingsStorage::getString("fota_fw_id",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.fota_fw_branch = UnifiedSettingsStorage::getInt(
    "fota_fw_branch", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.fota_fw_ts = UnifiedSettingsStorage::getInt64(
    "fota_fw_ts", atol(settingsKeyListData[i++].factoryValue));
  hubSettings.gnss_en = UnifiedSettingsStorage::getBool(
    "gnss_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.gnss_mode = UnifiedSettingsStorage::getInt(
    "gnss_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gnss_use_static = UnifiedSettingsStorage::getBool(
    "gnss_use_static", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.gnss_static_lat = UnifiedSettingsStorage::getFloat(
    "gnss_static_lat", atof(settingsKeyListData[i++].factoryValue));
  hubSettings.gnss_static_lon = UnifiedSettingsStorage::getFloat(
    "gnss_static_lon", atof(settingsKeyListData[i++].factoryValue));
  hubSettings.gnss_static_alt = UnifiedSettingsStorage::getInt(
    "gnss_static_alt", atoi(settingsKeyListData[i++].factoryValue));
#if defined(LAYRZ_HUB1_BUILD) || defined(LAYRZ_HUB2_BUILD)
  hubSettings.gpio_1_mode = UnifiedSettingsStorage::getInt(
    "gpio_1_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_2_mode = UnifiedSettingsStorage::getInt(
    "gpio_2_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_3_mode = UnifiedSettingsStorage::getInt(
    "gpio_3_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_4_mode = UnifiedSettingsStorage::getInt(
    "gpio_4_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_5_mode = UnifiedSettingsStorage::getInt(
    "gpio_5_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_6_mode = UnifiedSettingsStorage::getInt(
    "gpio_6_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_7_mode = UnifiedSettingsStorage::getInt(
    "gpio_7_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_14_mode = UnifiedSettingsStorage::getInt(
    "gpio_14_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_19_mode = UnifiedSettingsStorage::getInt(
    "gpio_19_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_20_mode = UnifiedSettingsStorage::getInt(
    "gpio_20_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_41_mode = UnifiedSettingsStorage::getInt(
    "gpio_41_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_42_mode = UnifiedSettingsStorage::getInt(
    "gpio_42_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_45_mode = UnifiedSettingsStorage::getInt(
    "gpio_45_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_46_mode = UnifiedSettingsStorage::getInt(
    "gpio_46_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_47_mode = UnifiedSettingsStorage::getInt(
    "gpio_47_mode", atoi(settingsKeyListData[i++].factoryValue));
#elif defined(LAYRZ_HUB25_BUILD)
  hubSettings.di1_mode = UnifiedSettingsStorage::getInt(
    "di1_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.di2_mode = UnifiedSettingsStorage::getInt(
    "di2_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.di3_mode = UnifiedSettingsStorage::getInt(
    "di3_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.di4_mode = UnifiedSettingsStorage::getInt(
    "di4_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.ai1_di5_mode = UnifiedSettingsStorage::getInt(
    "ai1_di5_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.ai2_di6_mode = UnifiedSettingsStorage::getInt(
    "ai2_di6_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_19_mode = UnifiedSettingsStorage::getInt(
    "gpio_19_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.gpio_20_mode = UnifiedSettingsStorage::getInt(
    "gpio_20_mode", atoi(settingsKeyListData[i++].factoryValue));
#endif

  // Skip BLE device settings - they are loaded dynamically in
  // BleDevicesLayrzHub::init()
  for (int bleIdx = 0; bleIdx < 50; bleIdx++) {
    i += 2; // Skip model and address entries in settingsKeyListData
  }

  hubSettings.uart_en = UnifiedSettingsStorage::getBool(
    "uart_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.uart_rx_pin = UnifiedSettingsStorage::getInt(
    "uart_rx_pin", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.uart_tx_pin = UnifiedSettingsStorage::getInt(
    "uart_tx_pin", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.uart_mode = UnifiedSettingsStorage::getString(
                            "uart_mode", settingsKeyListData[i++].factoryValue)
                            .c_str();
  hubSettings.uart_buffer = UnifiedSettingsStorage::getInt(
    "uart_buffer", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.uart_start =
    UnifiedSettingsStorage::getString("uart_start",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.uart_end = UnifiedSettingsStorage::getString(
                           "uart_end", settingsKeyListData[i++].factoryValue)
                           .c_str();
  hubSettings.uart_baud = UnifiedSettingsStorage::getInt(
    "uart_baud", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.uart_bits = UnifiedSettingsStorage::getInt(
    "uart_bits", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.uart_par = UnifiedSettingsStorage::getString(
                           "uart_par", settingsKeyListData[i++].factoryValue)
                           .c_str();
  hubSettings.rs485_en = UnifiedSettingsStorage::getBool(
    "rs485_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.rs485_delay = UnifiedSettingsStorage::getInt(
    "rs485_delay", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rs485_mode =
    UnifiedSettingsStorage::getString("rs485_mode",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.rs485_buffer = UnifiedSettingsStorage::getInt(
    "rs485_buffer", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rs485_start =
    UnifiedSettingsStorage::getString("rs485_start",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.rs485_end = UnifiedSettingsStorage::getString(
                            "rs485_end", settingsKeyListData[i++].factoryValue)
                            .c_str();
  hubSettings.rs485_baud = UnifiedSettingsStorage::getInt(
    "rs485_baud", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rs485_bits = UnifiedSettingsStorage::getInt(
    "rs485_bits", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rs485_par = UnifiedSettingsStorage::getString(
                            "rs485_par", settingsKeyListData[i++].factoryValue)
                            .c_str();
  hubSettings.rs232_1_en = UnifiedSettingsStorage::getBool(
    "rs232_1_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.rs232_1_mode =
    UnifiedSettingsStorage::getString("rs232_1_mode",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.rs232_1_buffer = UnifiedSettingsStorage::getInt(
    "rs232_1_buffer", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rs232_1_start =
    UnifiedSettingsStorage::getString("rs232_1_start",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.rs232_1_end =
    UnifiedSettingsStorage::getString("rs232_1_end",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.rs232_1_baud = UnifiedSettingsStorage::getInt(
    "rs232_1_baud", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rs232_1_bits = UnifiedSettingsStorage::getInt(
    "rs232_1_bits", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rs232_1_par =
    UnifiedSettingsStorage::getString("rs232_1_par",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.rs232_2_en = UnifiedSettingsStorage::getBool(
    "rs232_2_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.rs232_2_mode =
    UnifiedSettingsStorage::getString("rs232_2_mode",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.rs232_2_buffer = UnifiedSettingsStorage::getInt(
    "rs232_2_buffer", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rs232_2_start =
    UnifiedSettingsStorage::getString("rs232_2_start",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.rs232_2_end =
    UnifiedSettingsStorage::getString("rs232_2_end",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.rs232_2_baud = UnifiedSettingsStorage::getInt(
    "rs232_2_baud", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rs232_2_bits = UnifiedSettingsStorage::getInt(
    "rs232_2_bits", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rs232_2_par =
    UnifiedSettingsStorage::getString("rs232_2_par",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.ri_api_url =
    UnifiedSettingsStorage::getString("ri_api_url",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.ri_api_token =
    UnifiedSettingsStorage::getString("ri_api_token",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.ri_api_ident =
    UnifiedSettingsStorage::getString("ri_api_ident",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.ri_api_permit =
    UnifiedSettingsStorage::getString("ri_api_permit",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.acc_en = UnifiedSettingsStorage::getBool(
    "acc_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.acc_mode = UnifiedSettingsStorage::getInt(
    "acc_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.acc_grav_filter = UnifiedSettingsStorage::getBool(
    "acc_grav_filter", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.acc_grav_thr = UnifiedSettingsStorage::getInt(
    "acc_grav_thr", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rgb_en = UnifiedSettingsStorage::getBool(
    "rgb_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.rgb_count = UnifiedSettingsStorage::getInt(
    "rgb_count", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.rgb_brightness = UnifiedSettingsStorage::getInt(
    "rgb_brightness", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.data_upd_per = UnifiedSettingsStorage::getInt(
    "data_upd_per", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.data_pub_per = UnifiedSettingsStorage::getInt(
    "data_pub_per", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.data_ble_en = UnifiedSettingsStorage::getBool(
    "data_ble_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.data_ble_dec_en = UnifiedSettingsStorage::getBool(
    "data_ble_dec_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.data_ble_win = UnifiedSettingsStorage::getInt(
    "data_ble_win", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.data_serial_en = UnifiedSettingsStorage::getBool(
    "data_serial_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.data_gpio_en = UnifiedSettingsStorage::getBool(
    "data_gpio_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.onewire_en = UnifiedSettingsStorage::getBool(
    "onewire_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.onewire_pin = UnifiedSettingsStorage::getInt(
    "onewire_pin", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.onewire_0_model =
    UnifiedSettingsStorage::getString("onewire_0_model",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_0_addr =
    UnifiedSettingsStorage::getString("onewire_0_addr",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_1_model =
    UnifiedSettingsStorage::getString("onewire_1_model",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_1_addr =
    UnifiedSettingsStorage::getString("onewire_1_addr",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_2_model =
    UnifiedSettingsStorage::getString("onewire_2_model",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_2_addr =
    UnifiedSettingsStorage::getString("onewire_2_addr",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_3_model =
    UnifiedSettingsStorage::getString("onewire_3_model",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_3_addr =
    UnifiedSettingsStorage::getString("onewire_3_addr",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_4_model =
    UnifiedSettingsStorage::getString("onewire_4_model",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_4_addr =
    UnifiedSettingsStorage::getString("onewire_4_addr",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_5_model =
    UnifiedSettingsStorage::getString("onewire_5_model",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.onewire_5_addr =
    UnifiedSettingsStorage::getString("onewire_5_addr",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.hpc_front_addr = UnifiedSettingsStorage::getInt(
    "hpc_front_addr", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.hpc_back_addr = UnifiedSettingsStorage::getInt(
    "hpc_back_addr", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.hpc_back_en = UnifiedSettingsStorage::getBool(
    "hpc_back_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.can_en = UnifiedSettingsStorage::getBool(
    "can_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.can_mode = UnifiedSettingsStorage::getInt(
    "can_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.can_decode_en = UnifiedSettingsStorage::getBool(
    "can_decode_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.can_j1939_filt = UnifiedSettingsStorage::getInt(
    "can_j1939_filt", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.can_obd2_filt = UnifiedSettingsStorage::getInt(
    "can_obd2_filt", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.can_dtc_en = UnifiedSettingsStorage::getBool(
    "can_dtc_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.can_j1939_dtc_f = UnifiedSettingsStorage::getInt(
    "can_j1939_dtc_f", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.can_obd2_dtc_f = UnifiedSettingsStorage::getInt(
    "can_obd2_dtc_f", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.can_poll_win = UnifiedSettingsStorage::getInt(
    "can_poll_win", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.can_pub_per = UnifiedSettingsStorage::getInt(
    "can_pub_per", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.can_nan_filter = UnifiedSettingsStorage::getBool(
    "can_nan_filter", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.dn23e08_s_addr = UnifiedSettingsStorage::getInt(
    "dn23e08_s_addr", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.dn23e08_e_addr = UnifiedSettingsStorage::getInt(
    "dn23e08_e_addr", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.acam_en = UnifiedSettingsStorage::getBool(
    "acam_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.acam_name = UnifiedSettingsStorage::getString(
                            "acam_name", settingsKeyListData[i++].factoryValue)
                            .c_str();
  hubSettings.acam_model = UnifiedSettingsStorage::getInt(
    "acam_model", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.acam_res = UnifiedSettingsStorage::getInt(
    "acam_res", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.acam_wb_mode = UnifiedSettingsStorage::getInt(
    "acam_wb_mode", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.acam_color_fx = UnifiedSettingsStorage::getInt(
    "acam_color_fx", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.acam_trig_src = UnifiedSettingsStorage::getInt(
    "acam_trig_src", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.acam_trig_cnt = UnifiedSettingsStorage::getInt(
    "acam_trig_cnt", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.acam_trig_int = UnifiedSettingsStorage::getInt(
    "acam_trig_int", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.acam_cyclic_en = UnifiedSettingsStorage::getBool(
    "acam_cyclic_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.acam_cyclic_int = UnifiedSettingsStorage::getInt(
    "acam_cyclic_int", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.acam_storage_en = UnifiedSettingsStorage::getBool(
    "acam_storage_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.modbus_addr1 = UnifiedSettingsStorage::getInt(
    "modbus_addr1", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.modbus_addr2 = UnifiedSettingsStorage::getInt(
    "modbus_addr2", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.modbus_addr3 = UnifiedSettingsStorage::getInt(
    "modbus_addr3", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.modbus_addr4 = UnifiedSettingsStorage::getInt(
    "modbus_addr4", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.modbus_dev1 = UnifiedSettingsStorage::getInt(
    "modbus_dev1", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.modbus_dev2 = UnifiedSettingsStorage::getInt(
    "modbus_dev2", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.modbus_dev3 = UnifiedSettingsStorage::getInt(
    "modbus_dev3", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.modbus_dev4 = UnifiedSettingsStorage::getInt(
    "modbus_dev4", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.modbus_poll_int = UnifiedSettingsStorage::getInt(
    "modbus_poll_int", atoi(settingsKeyListData[i++].factoryValue));
  hubSettings.modbus_dec_en = UnifiedSettingsStorage::getBool(
    "modbus_dec_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.zigbee_en = UnifiedSettingsStorage::getBool(
    "zigbee_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.zigbee_local_en = UnifiedSettingsStorage::getBool(
    "zigbee_local_en", settingsKeyListData[i++].factoryValue == "true");
  hubSettings.zigbee_hub_url =
    UnifiedSettingsStorage::getString("zigbee_hub_url",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.zigbee_site_id =
    UnifiedSettingsStorage::getString("zigbee_site_id",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();
  hubSettings.zigbee_token =
    UnifiedSettingsStorage::getString("zigbee_token",
                                      settingsKeyListData[i++].factoryValue)
      .c_str();

  if (hubSettings.zigbee_local_en && hubSettings.sys_debug_en) {
    hubSettings.sys_debug_en = false;
  }

  // Settings loaded from UnifiedSettingsStorage - no need to close anything
}

int deviceSettings::getKeyIndex(const char *key) {
  for (int i = 0; i < settingsKeyListSize; i++) {
    if (strcmp(key, settingsKeyListData[i].key) == 0) {
      return i;
    }
  }
  return -1;
}

bool deviceSettings::setDeviceSetting(const char *key, const char *value) {
  // Check if key exists in our settings schema
  int index = getKeyIndex(key);
  if (index == -1) {
    debugPrint("setting parameter %s not defined in device\n", key);
    return false; // Key not found
  }

  bool success = false;
  String keyStr = String(key);

  // Use unified storage system for all settings
  if (settingsKeyListData[index].type == settingsKeyList::STRING) {
    success = UnifiedSettingsStorage::setString(keyStr, String(value));
    debugPrint("Setting key %s with value %s\n", key, value);
  } else if (settingsKeyListData[index].type == settingsKeyList::BOOL) {
    if (strcmp(value, "1") == 0 || strcmp(value, "0") == 0 ||
        strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
      bool boolValue = (strcmp(value, "1") == 0 || strcmp(value, "true") == 0);

      if (keyStr == "sys_debug_en" && hubSettings.zigbee_local_en &&
          boolValue) {
        boolValue = false;
      }

      success = UnifiedSettingsStorage::setBool(keyStr, boolValue);
      if (success && keyStr == "zigbee_local_en") {
        hubSettings.zigbee_local_en = boolValue;
        if (boolValue) {
          hubSettings.sys_debug_en = false;
          success = UnifiedSettingsStorage::setBool("sys_debug_en", false);
        }
      } else if (success && keyStr == "sys_debug_en") {
        hubSettings.sys_debug_en = boolValue;
      }
      debugPrint("Setting key %s with value %s\n", key, value);
    }
  } else if (settingsKeyListData[index].type == settingsKeyList::INT) {
    int intVal;
    if (sscanf(value, "%d", &intVal) == 1) { // Ensure exact conversion
      success = UnifiedSettingsStorage::setInt(keyStr, intVal);
    }
  } else if (settingsKeyListData[index].type == settingsKeyList::INT64) {
    int64_t int64Val;
    if (sscanf(value, "%lld", &int64Val) == 1) { // Ensure exact conversion
      success = UnifiedSettingsStorage::setInt64(keyStr, int64Val);
      debugPrint("Setting key %s with value %lld\n", key, int64Val);
    }
  } else if (settingsKeyListData[index].type == settingsKeyList::FLOAT) {
    float floatVal;
    if (sscanf(value, "%f", &floatVal) == 1) { // Ensure exact conversion
      success = UnifiedSettingsStorage::setFloat(keyStr, floatVal);
      debugPrint("Setting key %s with value %f\n", key, floatVal);
    }
  }

  if (!success) {
    debugPrint("Error setting key %s with value %s\n", key, value);
    return false;
  }

  // Don't save immediately - let caller handle batch saving
  return true;
}

std::string deviceSettings::getDeviceSettings() {
  std::string payload = "";
  initDeviceSettings();
  payload += "sys.dev.id:" + std::to_string(hubSettings.sys_dev_id) + ",";
  payload += "sys.dev.name:" + hubSettings.sys_dev_name + ",";
  payload += "sys.dev.hw.id:" + std::to_string(hubSettings.sys_dev_hw_id) + ",";
  payload += "sys.dev.md.id:" + std::to_string(hubSettings.sys_dev_md_id) + ",";
  payload += "sys.cmd.per:" + std::to_string(hubSettings.sys_cmd_per) + ",";
  payload +=
    "sys.debug.en:" +
    (hubSettings.sys_debug_en ? std::string("true") : std::string("false")) +
    ",";
  payload +=
    "sys.ntp.period:" + std::to_string(hubSettings.sys_ntp_period) + ",";
  payload += "sys.ntpserver1:" + hubSettings.sys_ntpserver1 + ",";
  payload += "sys.ntpserver2:" + hubSettings.sys_ntpserver2 + ",";
  payload +=
    "sys.ble.timeout:" + std::to_string(hubSettings.sys_ble_timeout) + ",";
#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
  payload +=
    "sys.hist.en:" +
    (hubSettings.sys_hist_en ? std::string("true") : std::string("false")) +
    ",";
#endif
  payload += "net.en:" +
             (hubSettings.net_en ? std::string("true") : std::string("false")) +
             ",";
  payload += "net.mode:" + hubSettings.net_mode + ",";
  payload += "net.server:" + hubSettings.net_server + ",";
  payload += "net.protocol:" + std::to_string(hubSettings.net_protocol) + ",";
  payload += "wifi.ssid:" + hubSettings.wifi_ssid + ",";
  payload += "wifi.pass:" + hubSettings.wifi_pass + ",";
  payload += "gprs.apn:" + hubSettings.gprs_apn + ",";
  payload += "gprs.apn.user:" + hubSettings.gprs_apn_user + ",";
  payload += "gprs.apn.pass:" + hubSettings.gprs_apn_pass + ",";
  payload += "gprs.pin:" + hubSettings.gprs_pin + ",";
  payload +=
    "fota.en:" +
    (hubSettings.fota_en ? std::string("true") : std::string("false")) + ",";
  payload +=
    "fota.force:" +
    (hubSettings.fota_force ? std::string("true") : std::string("false")) + ",";
  payload += "fota.fw.id:" + hubSettings.fota_fw_id + ",";
  payload +=
    "fota.fw.branch:" + std::to_string(hubSettings.fota_fw_branch) + ",";
  payload += "fota.fw.ts:" + std::to_string(hubSettings.fota_fw_ts) + ",";
  payload +=
    "gnss.en:" +
    (hubSettings.gnss_en ? std::string("true") : std::string("false")) + ",";
  payload += "gnss.mode:" + std::to_string(hubSettings.gnss_mode) + ",";
  payload +=
    "gnss.use.static:" +
    (hubSettings.gnss_use_static ? std::string("true") : std::string("false")) +
    ",";
  payload +=
    "gnss.static.lat:" + std::to_string(hubSettings.gnss_static_lat) + ",";
  payload +=
    "gnss.static.lon:" + std::to_string(hubSettings.gnss_static_lon) + ",";
  payload +=
    "gnss.static.alt:" + std::to_string(hubSettings.gnss_static_alt) + ",";
#if defined(LAYRZ_HUB1_BUILD)
  payload += "gpio.1.mode:" + std::to_string(hubSettings.gpio_1_mode) + ",";
  payload += "gpio.2.mode:" + std::to_string(hubSettings.gpio_2_mode) + ",";
  payload += "gpio.5.mode:" + std::to_string(hubSettings.gpio_5_mode) + ",";
  payload += "gpio.6.mode:" + std::to_string(hubSettings.gpio_6_mode) + ",";
  payload += "gpio.7.mode:" + std::to_string(hubSettings.gpio_7_mode) + ",";
  payload += "gpio.14.mode:" + std::to_string(hubSettings.gpio_14_mode) + ",";
  payload += "gpio.45.mode:" + std::to_string(hubSettings.gpio_45_mode) + ",";
  payload += "gpio.46.mode:" + std::to_string(hubSettings.gpio_46_mode) + ",";
  payload += "gpio.47.mode:" + std::to_string(hubSettings.gpio_47_mode) + ",";
#elif defined(LAYRZ_HUB2_BUILD)
  payload += "gpio.3.mode:" + std::to_string(hubSettings.gpio_3_mode) + ",";
  payload += "gpio.4.mode:" + std::to_string(hubSettings.gpio_4_mode) + ",";
  payload += "gpio.5.mode:" + std::to_string(hubSettings.gpio_5_mode) + ",";
  payload += "gpio.6.mode:" + std::to_string(hubSettings.gpio_6_mode) + ",";
  payload += "gpio.19.mode:" + std::to_string(hubSettings.gpio_19_mode) + ",";
  payload += "gpio.20.mode:" + std::to_string(hubSettings.gpio_20_mode) + ",";
  payload += "gpio.41.mode:" + std::to_string(hubSettings.gpio_41_mode) + ",";
  payload += "gpio.42.mode:" + std::to_string(hubSettings.gpio_42_mode) + ",";
#elif defined(LAYRZ_HUB25_BUILD)
  payload += "di1.mode:" + std::to_string(hubSettings.di1_mode) + ",";
  payload += "di2.mode:" + std::to_string(hubSettings.di2_mode) + ",";
  payload += "di3.mode:" + std::to_string(hubSettings.di3_mode) + ",";
  payload += "di4.mode:" + std::to_string(hubSettings.di4_mode) + ",";
  payload += "ai1_di5.mode:" + std::to_string(hubSettings.ai1_di5_mode) + ",";
  payload += "ai2_di6.mode:" + std::to_string(hubSettings.ai2_di6_mode) + ",";
  payload += "gpio.19.mode:" + std::to_string(hubSettings.gpio_19_mode) + ",";
  payload += "gpio.20.mode:" + std::to_string(hubSettings.gpio_20_mode) + ",";
#endif
  // Dynamic loop for 50 BLE devices using unified storage
  for (int i = 0; i < 50; i++) {
    String modelKey = "ble_" + String(i) + "_model";
    String addressKey = "ble_" + String(i) + "_address";
    String modelValue = UnifiedSettingsStorage::getString(modelKey, "");
    String addressValue = UnifiedSettingsStorage::getString(addressKey, "");

    payload +=
      "ble." + std::to_string(i) + ".model:" + modelValue.c_str() + ",";
    payload +=
      "ble." + std::to_string(i) + ".address:" + addressValue.c_str() + ",";
  }
  payload +=
    "uart.en:" +
    (hubSettings.uart_en ? std::string("true") : std::string("false")) + ",";
  payload += "uart.rx.pin:" + std::to_string(hubSettings.uart_rx_pin) + ",";
  payload += "uart.tx.pin:" + std::to_string(hubSettings.uart_tx_pin) + ",";
  payload += "uart.mode:" + hubSettings.uart_mode + ",";
  payload += "uart.buffer:" + std::to_string(hubSettings.uart_buffer) + ",";
  payload += "uart.start:" + hubSettings.uart_start + ",";
  payload += "uart.end:" + hubSettings.uart_end + ",";
  payload += "uart.baud:" + std::to_string(hubSettings.uart_baud) + ",";
  payload += "uart.bits:" + std::to_string(hubSettings.uart_bits) + ",";
  payload += "uart.par:" + hubSettings.uart_par + ",";
  payload +=
    "rs485.en:" +
    (hubSettings.rs485_en ? std::string("true") : std::string("false")) + ",";
  payload += "rs485.delay:" + std::to_string(hubSettings.rs485_delay) + ",";
  payload += "rs485.mode:" + hubSettings.rs485_mode + ",";
  payload += "rs485.buffer:" + std::to_string(hubSettings.rs485_buffer) + ",";
  payload += "rs485.start:" + hubSettings.rs485_start + ",";
  payload += "rs485.end:" + hubSettings.rs485_end + ",";
  payload += "rs485.baud:" + std::to_string(hubSettings.rs485_baud) + ",";
  payload += "rs485.bits:" + std::to_string(hubSettings.rs485_bits) + ",";
  payload += "rs485.par:" + hubSettings.rs485_par + ",";
  payload +=
    "rs232.1.en:" +
    (hubSettings.rs232_1_en ? std::string("true") : std::string("false")) + ",";
  payload += "rs232.1.mode:" + hubSettings.rs232_1_mode + ",";
  payload +=
    "rs232.1.buffer:" + std::to_string(hubSettings.rs232_1_buffer) + ",";
  payload += "rs232.1.start:" + hubSettings.rs232_1_start + ",";
  payload += "rs232.1.end:" + hubSettings.rs232_1_end + ",";
  payload += "rs232.1.baud:" + std::to_string(hubSettings.rs232_1_baud) + ",";
  payload += "rs232.1.bits:" + std::to_string(hubSettings.rs232_1_bits) + ",";
  payload += "rs232.1.par:" + hubSettings.rs232_1_par + ",";
  payload +=
    "rs232.2.en:" +
    (hubSettings.rs232_2_en ? std::string("true") : std::string("false")) + ",";
  payload += "rs232.2.mode:" + hubSettings.rs232_2_mode + ",";
  payload +=
    "rs232.2.buffer:" + std::to_string(hubSettings.rs232_2_buffer) + ",";
  payload += "rs232.2.start:" + hubSettings.rs232_2_start + ",";
  payload += "rs232.2.end:" + hubSettings.rs232_2_end + ",";
  payload += "rs232.2.baud:" + std::to_string(hubSettings.rs232_2_baud) + ",";
  payload += "rs232.2.bits:" + std::to_string(hubSettings.rs232_2_bits) + ",";
  payload += "rs232.2.par:" + hubSettings.rs232_2_par + ",";
  payload += "ri.api.url:" + hubSettings.ri_api_url + ",";
  payload += "ri.api.token:" + hubSettings.ri_api_token + ",";
  payload += "ri.api.ident:" + hubSettings.ri_api_ident + ",";
  payload += "ri.api.permit:" + hubSettings.ri_api_permit + ",";
  payload += "acc.en:" +
             (hubSettings.acc_en ? std::string("true") : std::string("false")) +
             ",";
  payload += "acc.mode:" + std::to_string(hubSettings.acc_mode) + ",";
  payload +=
    "acc.grav.filter:" +
    (hubSettings.acc_grav_filter ? std::string("true") : std::string("false")) +
    ",";
  payload += "acc.grav.thr:" + std::to_string(hubSettings.acc_grav_thr) + ",";
  payload += "rgb.en:" +
             (hubSettings.rgb_en ? std::string("true") : std::string("false")) +
             ",";
  payload += "rgb.count:" + std::to_string(hubSettings.rgb_count) + ",";
  payload +=
    "rgb.brightness:" + std::to_string(hubSettings.rgb_brightness) + ",";
  payload += "data.upd.per:" + std::to_string(hubSettings.data_upd_per) + ",";
  payload += "data.pub.per:" + std::to_string(hubSettings.data_pub_per) + ",";
  payload +=
    "data.ble.en:" +
    (hubSettings.data_ble_en ? std::string("true") : std::string("false")) +
    ",";
  payload +=
    "data.ble.dec.en:" +
    (hubSettings.data_ble_dec_en ? std::string("true") : std::string("false")) +
    ",";
  payload += "data.ble.win:" + std::to_string(hubSettings.data_ble_win) + ",";
  payload +=
    "data.serial.en:" +
    (hubSettings.data_serial_en ? std::string("true") : std::string("false")) +
    ",";
  payload +=
    "data.gpio.en:" +
    (hubSettings.data_gpio_en ? std::string("true") : std::string("false")) +
    ",";
  payload +=
    "onewire.en:" +
    (hubSettings.onewire_en ? std::string("true") : std::string("false")) + ",";
  payload += "onewire.pin:" + std::to_string(hubSettings.onewire_pin) + ",";
  payload += "onewire.0.model:" + hubSettings.onewire_0_model + ",";
  payload += "onewire.0.addr:" + hubSettings.onewire_0_addr + ",";
  payload += "onewire.1.model:" + hubSettings.onewire_1_model + ",";
  payload += "onewire.1.addr:" + hubSettings.onewire_1_addr + ",";
  payload += "onewire.2.model:" + hubSettings.onewire_2_model + ",";
  payload += "onewire.2.addr:" + hubSettings.onewire_2_addr + ",";
  payload += "onewire.3.model:" + hubSettings.onewire_3_model + ",";
  payload += "onewire.3.addr:" + hubSettings.onewire_3_addr + ",";
  payload += "onewire.4.model:" + hubSettings.onewire_4_model + ",";
  payload += "onewire.4.addr:" + hubSettings.onewire_4_addr + ",";
  payload += "onewire.5.model:" + hubSettings.onewire_5_model + ",";
  payload += "onewire.5.addr:" + hubSettings.onewire_5_addr + ",";
  payload +=
    "hpc.front.addr:" + std::to_string(hubSettings.hpc_front_addr) + ",";
  payload += "hpc.back.addr:" + std::to_string(hubSettings.hpc_back_addr) + ",";
  payload +=
    "hpc.back.en:" +
    (hubSettings.hpc_back_en ? std::string("true") : std::string("false")) +
    ",";
  payload += "can.en:" +
             (hubSettings.can_en ? std::string("true") : std::string("false")) +
             ",";
  payload += "can.mode:" + std::to_string(hubSettings.can_mode) + ",";
  payload +=
    "can.decode.en:" +
    (hubSettings.can_decode_en ? std::string("true") : std::string("false")) +
    ",";
  payload +=
    "can.j1939.filt:" + std::to_string(hubSettings.can_j1939_filt) + ",";
  payload += "can.obd2.filt:" + std::to_string(hubSettings.can_obd2_filt) + ",";
  payload +=
    "can.dtc.en:" +
    (hubSettings.can_dtc_en ? std::string("true") : std::string("false")) + ",";
  payload +=
    "can.j1939.dtc.f:" + std::to_string(hubSettings.can_j1939_dtc_f) + ",";
  payload +=
    "can.obd2.dtc.f:" + std::to_string(hubSettings.can_obd2_dtc_f) + ",";
  payload += "can.poll.win:" + std::to_string(hubSettings.can_poll_win) + ",";
  payload += "can.pub.per:" + std::to_string(hubSettings.can_pub_per) + ",";
  payload +=
    "can.nan.filter:" +
    (hubSettings.can_nan_filter ? std::string("true") : std::string("false")) +
    ",";
  payload +=
    "dn23e08.s.addr:" + std::to_string(hubSettings.dn23e08_s_addr) + ",";
  payload +=
    "dn23e08.e.addr:" + std::to_string(hubSettings.dn23e08_e_addr) + ",";
  payload +=
    "acam.en:" +
    (hubSettings.acam_en ? std::string("true") : std::string("false")) + ",";
  payload += "acam.name:" + hubSettings.acam_name + ",";
  payload += "acam.model:" + std::to_string(hubSettings.acam_model) + ",";
  payload += "acam.res:" + std::to_string(hubSettings.acam_res) + ",";
  payload += "acam.wb.mode:" + std::to_string(hubSettings.acam_wb_mode) + ",";
  payload += "acam.color.fx:" + std::to_string(hubSettings.acam_color_fx) + ",";
  payload += "acam.trig.src:" + std::to_string(hubSettings.acam_trig_src) + ",";
  payload += "acam.trig.cnt:" + std::to_string(hubSettings.acam_trig_cnt) + ",";
  payload += "acam.trig.int:" + std::to_string(hubSettings.acam_trig_int) + ",";
  payload +=
    "acam.cyclic.en:" +
    (hubSettings.acam_cyclic_en ? std::string("true") : std::string("false")) +
    ",";
  payload +=
    "acam.cyclic.int:" + std::to_string(hubSettings.acam_cyclic_int) + ",";
#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
  payload +=
    "acam.storage.en:" +
    (hubSettings.acam_storage_en ? std::string("true") : std::string("false")) +
    ",";
#endif
  payload += "modbus.addr1:" + std::to_string(hubSettings.modbus_addr1) + ",";
  payload += "modbus.addr2:" + std::to_string(hubSettings.modbus_addr2) + ",";
  payload += "modbus.addr3:" + std::to_string(hubSettings.modbus_addr3) + ",";
  payload += "modbus.addr4:" + std::to_string(hubSettings.modbus_addr4) + ",";
  payload += "modbus.dev1:" + std::to_string(hubSettings.modbus_dev1) + ",";
  payload += "modbus.dev2:" + std::to_string(hubSettings.modbus_dev2) + ",";
  payload += "modbus.dev3:" + std::to_string(hubSettings.modbus_dev3) + ",";
  payload += "modbus.dev4:" + std::to_string(hubSettings.modbus_dev4) + ",";
  payload +=
    "modbus.poll.int:" + std::to_string(hubSettings.modbus_poll_int) + ",";
  payload +=
    "modbus.dec.en:" +
    (hubSettings.modbus_dec_en ? std::string("true") : std::string("false")) +
    ",";
  payload +=
    "zigbee.en:" +
    (hubSettings.zigbee_en ? std::string("true") : std::string("false")) + ",";
  payload +=
    "zigbee.local.en:" +
    (hubSettings.zigbee_local_en ? std::string("true") : std::string("false")) +
    ",";
  payload += "zigbee.hub.url:" + hubSettings.zigbee_hub_url + ",";
  payload += "zigbee.site.id:" + hubSettings.zigbee_site_id + ",";
  payload += "zigbee.token:" + hubSettings.zigbee_token;
  return payload;
}

std::string deviceSettings::getDeviceShortSettings() {
  initDeviceSettings();
  std::string payload = "";
  payload += "net.en:" +
             (hubSettings.net_en ? std::string("true") : std::string("false")) +
             ",";
  payload += "net.mode:" + hubSettings.net_mode + ",";
  payload += "wifi.ssid:" + hubSettings.wifi_ssid + ",";
  payload += "wifi.pass:" + hubSettings.wifi_pass + ",";
  payload += "gprs.apn:" + hubSettings.gprs_apn + ",";
  payload += "gprs.apn.user:" + hubSettings.gprs_apn_user + ",";
  payload += "gprs.apn.pass:" + hubSettings.gprs_apn_pass + ",";
  payload += "gprs.pin:" + hubSettings.gprs_pin;
  return payload;
}

void deviceSettings::setFactorySettings() {
  // Clear all settings and reinitialize with defaults using the new unified
  // system
  if (UnifiedSettingsStorage::clear()) {
    if (UnifiedSettingsStorage::saveAllSettings()) {
      debugPrint("Factory settings restored and saved successfully\n");
      initDeviceSettings();
    } else {
      debugPrint("Failed to save factory settings\n");
    }
  } else {
    debugPrint("Failed to clear settings\n");
  }
}

void deviceSettings::initSettingsBuffer() {
  settingsBuffer =
    (char *)heap_caps_malloc(SETTINGS_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (settingsBuffer == nullptr) {
    debugPrint("Failed to allocate settings buffer size\n");
    return;
  }
  memset(settingsBuffer, 0, SETTINGS_BUFFER_SIZE); // initialize with zeros
}

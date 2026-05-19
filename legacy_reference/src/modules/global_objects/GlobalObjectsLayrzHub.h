#pragma once

#ifndef __GLOBALOBJECTSLAYRZHUB_H__
#define __GLOBALOBJECTSLAYRZHUB_H__

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FreeRTOS.h>
#include <NimBLECharacteristic.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEServer.h>
#include <NimBLEService.h>
#include <NimBLEUtils.h>
#include <TimeLib.h>
#include <USB.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_task_wdt.h>
#include <iomanip> // For std::setw and std::setfill
#include <memory>
#include <modules/blackbox/BlackBoxLayrzHub.h>
#include <modules/definitions/DefinitionsLayrzHub.h>
#include <modules/utilities/UtilitiesLayrzHub.h>
#include <sstream>
#include <unordered_map>
#include <vector>
// #include "decoder.h"

#include <AsyncA7670.h>
#include <AsyncA7670Client.h>
#include <SSLClient.h>
#include <TinyGSM.h>
#include <WiFi.h>
#include <WiFiClient.h>
// #include <ESP_SSLClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoHttpClient.h>
#include <HardwareSerial.h>
#include <Preferences.h>
#include <SoftwareSerial.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

struct GPIOConfig {
  int pin;
  int mode;
};

struct SpiRamAllocator {
  void *allocate(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }

  void deallocate(void *ptr) { heap_caps_free(ptr); }

  void *reallocate(void *ptr, size_t new_size) {
    return heap_caps_realloc(ptr, new_size,
                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
};

// Settings struct
typedef struct {
  uint64_t sys_dev_id;
  std::string sys_dev_name;
  uint16_t sys_dev_hw_id;
  uint64_t sys_dev_md_id;
  int sys_cmd_per;
  bool sys_debug_en;
  int sys_ntp_period;
  std::string sys_ntpserver1;
  std::string sys_ntpserver2;
  int sys_ble_timeout;
  bool sys_hist_en;
  bool net_en;
  std::string net_mode;
  std::string net_server;
  int net_protocol;
  std::string wifi_ssid;
  std::string wifi_pass;
  std::string gprs_apn;
  std::string gprs_apn_user;
  std::string gprs_apn_pass;
  std::string gprs_pin;
  bool fota_en;
  bool fota_force;
  std::string fota_fw_id;
  int fota_fw_branch;
  long fota_fw_ts;
  bool gnss_en;
  int gnss_mode;
  bool gnss_use_static;
  float gnss_static_lat;
  float gnss_static_lon;
  int gnss_static_alt;
#if defined(LAYRZ_HUB1_BUILD) || defined(LAYRZ_HUB2_BUILD)
  int gpio_1_mode;
  int gpio_2_mode;
  int gpio_3_mode;
  int gpio_4_mode;
  int gpio_5_mode;
  int gpio_6_mode;
  int gpio_7_mode;
  int gpio_14_mode;
  int gpio_19_mode;
  int gpio_20_mode;
  int gpio_41_mode;
  int gpio_42_mode;
  int gpio_45_mode;
  int gpio_46_mode;
  int gpio_47_mode;
#elif defined(LAYRZ_HUB25_BUILD)
  int di1_mode;
  int di2_mode;
  int di3_mode;
  int di4_mode;
  int ai1_di5_mode;
  int ai2_di6_mode;
  int gpio_19_mode;
  int gpio_20_mode;
#endif
  std::string ble_0_model;
  std::string ble_0_address;
  std::string ble_1_model;
  std::string ble_1_address;
  std::string ble_2_model;
  std::string ble_2_address;
  std::string ble_3_model;
  std::string ble_3_address;
  std::string ble_4_model;
  std::string ble_4_address;
  std::string ble_5_model;
  std::string ble_5_address;
  std::string ble_6_model;
  std::string ble_6_address;
  std::string ble_7_model;
  std::string ble_7_address;
  std::string ble_8_model;
  std::string ble_8_address;
  std::string ble_9_model;
  std::string ble_9_address;
  bool uart_en;
  int uart_rx_pin;
  int uart_tx_pin;
  std::string uart_mode;
  int uart_buffer;
  std::string uart_start;
  std::string uart_end;
  int uart_baud;
  int uart_bits;
  std::string uart_par;
  bool rs485_en;
  int rs485_delay;
  std::string rs485_mode;
  int rs485_buffer;
  std::string rs485_start;
  std::string rs485_end;
  int rs485_baud;
  int rs485_bits;
  std::string rs485_par;
  bool rs232_1_en;
  std::string rs232_1_mode;
  int rs232_1_buffer;
  std::string rs232_1_start;
  std::string rs232_1_end;
  int rs232_1_baud;
  int rs232_1_bits;
  std::string rs232_1_par;
  bool rs232_2_en;
  std::string rs232_2_mode;
  int rs232_2_buffer;
  std::string rs232_2_start;
  std::string rs232_2_end;
  int rs232_2_baud;
  int rs232_2_bits;
  std::string rs232_2_par;
  std::string ri_api_url;
  std::string ri_api_token;
  std::string ri_api_ident;
  std::string ri_api_permit;
  bool acc_en;
  int acc_mode;
  bool acc_grav_filter;
  int acc_grav_thr;
  bool rgb_en;
  int rgb_count;
  int rgb_brightness;
  int data_upd_per;
  int data_pub_per;
  boolean data_ble_en;
  boolean data_ble_dec_en;
  int data_ble_win;
  boolean data_serial_en;
  boolean data_gpio_en;
  boolean onewire_en;
  int onewire_pin;
  std::string onewire_0_model;
  std::string onewire_0_addr;
  std::string onewire_1_model;
  std::string onewire_1_addr;
  std::string onewire_2_model;
  std::string onewire_2_addr;
  std::string onewire_3_model;
  std::string onewire_3_addr;
  std::string onewire_4_model;
  std::string onewire_4_addr;
  std::string onewire_5_model;
  std::string onewire_5_addr;
  int hpc_front_addr;
  int hpc_back_addr;
  bool hpc_back_en;
  bool can_en;
  int can_mode;
  bool can_decode_en;
  int can_j1939_filt;
  int can_obd2_filt;
  bool can_dtc_en;
  int can_j1939_dtc_f;
  int can_obd2_dtc_f;
  int can_poll_win;
  int can_pub_per;
  bool can_nan_filter;
  int dn23e08_s_addr;
  int dn23e08_e_addr;
  bool acam_en;
  std::string acam_name;
  int acam_model;
  int acam_res;
  int acam_wb_mode;
  int acam_color_fx;
  int acam_trig_src;
  int acam_trig_cnt;
  int acam_trig_int;
  bool acam_cyclic_en;
  int acam_cyclic_int;
  bool acam_storage_en;
  int modbus_addr1;
  int modbus_addr2;
  int modbus_addr3;
  int modbus_addr4;
  int modbus_dev1;
  int modbus_dev2;
  int modbus_dev3;
  int modbus_dev4;
  int modbus_poll_int;
  bool modbus_dec_en;
  bool zigbee_en;
  bool zigbee_local_en;
  std::string zigbee_hub_url;
  std::string zigbee_site_id;
  std::string zigbee_token;
} settingsVars;

// BLE pair struct
typedef struct {
  std::string address;
  std::string model;
} blePair;

// Global variables externs
extern settingsVars hubSettings;
extern char *msgBuffer;
extern char *settingsBuffer;
extern char *uartIoDataBuffer;
extern char *rs485DataBuffer;
extern char *rs232_1DataBuffer;
extern char *rs232_2DataBuffer;
extern char *extIoDataBuffer;
extern char *canDataBuffer;
extern char *obd2DtcBuffer;
extern char *mediaMsgBuffer;
extern char *modbusMsgBuffer;
extern bool isObd2Initialized;
extern uint32_t supportedPids[150];
extern uint8_t supportedPidsCount;
extern time_t timestamp;
extern volatile bool requestObd2Dtcs;
extern volatile bool bleConfigConnected;
extern volatile bool httpBusy;
extern volatile uint32_t httpBusySinceMs;
extern volatile bool isFWChecked;
extern volatile bool isSocketAuth;
extern volatile bool isNtpSynced;
extern volatile bool isValidTime;
extern volatile bool netConnectFirstTime;
extern volatile bool socketSendSuccess;
extern volatile bool bootAfterCommand;
extern volatile bool rebootAfterPcAck;
extern char rebootAfterPcCmdId[64];
extern volatile bool arducamInitialized;
extern volatile bool sdCardInitialized;
extern volatile bool gBleStackReady;
extern long socketAuthTimeout;
// Calibration characteristics for each GPIO
#if defined(LAYRZ_HUB1_BUILD)
extern esp_adc_cal_characteristics_t *adc_chars_gpio1;
extern esp_adc_cal_characteristics_t *adc_chars_gpio2;
extern esp_adc_cal_characteristics_t *adc_chars_gpio5;
extern esp_adc_cal_characteristics_t *adc_chars_gpio6;
extern esp_adc_cal_characteristics_t *adc_chars_gpio7;
extern esp_adc_cal_characteristics_t *adc_chars_gpio14;
#elif defined(LAYRZ_HUB2_BUILD)
extern esp_adc_cal_characteristics_t *adc_chars_gpio3;
extern esp_adc_cal_characteristics_t *adc_chars_gpio4;
extern esp_adc_cal_characteristics_t *adc_chars_gpio5;
extern esp_adc_cal_characteristics_t *adc_chars_gpio6;
extern esp_adc_cal_characteristics_t *adc_chars_gpio7;
extern esp_adc_cal_characteristics_t *adc_chars_gpio19;
extern esp_adc_cal_characteristics_t *adc_chars_gpio20;
extern esp_adc_cal_characteristics_t *adc_chars_gpio41;
extern esp_adc_cal_characteristics_t *adc_chars_gpio42;
#elif defined(LAYRZ_HUB25_BUILD)
extern esp_adc_cal_characteristics_t *adc_chars_ai1;
extern esp_adc_cal_characteristics_t *adc_chars_ai2;
extern esp_adc_cal_characteristics_t *adc_chars_gpio19;
extern esp_adc_cal_characteristics_t *adc_chars_gpio20;
extern esp_adc_cal_characteristics_t *adc_chars_vbat;
#endif

#if defined(LAYRZ_HUB1_BUILD)
extern TimerHandle_t debTimerIO1;
extern TimerHandle_t debTimerIO2;
extern TimerHandle_t debTimerIO5;
extern TimerHandle_t debTimerIO6;
extern TimerHandle_t debTimerIO7;
extern TimerHandle_t debTimerIO14;
extern TimerHandle_t debTimerIO45;
extern TimerHandle_t debTimerIO46;
extern TimerHandle_t debTimerIO47;
#elif defined(LAYRZ_HUB2_BUILD)
extern TimerHandle_t debTimerIO3;
extern TimerHandle_t debTimerIO4;
extern TimerHandle_t debTimerIO5;
extern TimerHandle_t debTimerIO6;
extern TimerHandle_t debTimerIO19;
extern TimerHandle_t debTimerIO20;
extern TimerHandle_t debTimerIO41;
extern TimerHandle_t debTimerIO42;
#elif defined(LAYRZ_HUB25_BUILD)
extern TimerHandle_t debTimerIO4;
extern TimerHandle_t debTimerIO5;
extern TimerHandle_t debTimerIO6;
extern TimerHandle_t debTimerIO7;
extern TimerHandle_t debTimerIO8;
extern TimerHandle_t debTimerIO19;
extern TimerHandle_t debTimerIO20;
#endif
extern TimerHandle_t bleConfigTimer;
extern GPIOConfig gpioConfigs[10];
extern uint32_t gpioEventCounters[10];
extern blePair bleDevices[50];
// LEDC channel allocation (6 channels)
extern ledc_channel_t pwmChannels[6];

// Tasks handlers
extern TaskHandle_t checkFirmwareHandle;
extern TaskHandle_t updateSensorsHandle;
extern TaskHandle_t sendSensorDataHandle;
extern TaskHandle_t pingTaskHandle;
extern TaskHandle_t checkWifiNetworkHandle;
extern TaskHandle_t checkGSMNetworkHandle;
extern TaskHandle_t getCommandsHandle;
extern TaskHandle_t bleScanHandle;
extern TaskHandle_t serialMonitorHandle;
extern TaskHandle_t confiotOverBleHandle;
extern TaskHandle_t synchroNTPHandle;
extern TaskHandle_t tcpSocketReceptionHandle;
extern TaskHandle_t j1939MonitorTaskHandle;
extern TaskHandle_t obd2MonitorTaskHandle;
extern TaskHandle_t arducamTaskHandle;
extern TaskHandle_t systemHealthMonitorHandle;

// Global xTask externs
extern SemaphoreHandle_t xSemaphore;

// Serial objects externs
extern HardwareSerial UART_COMM;
extern HardwareSerial ZigSerial;
extern SoftwareSerial UART_IO;
extern SoftwareSerial RS232_1;
extern SoftwareSerial RS232_2;
extern SoftwareSerial RS485;

// Network objects externs
#ifdef USE_ASYNC_A7670
// Use AsyncA7670 library
extern AsyncA7670 modemAsync;
extern AsyncA7670Client GSMClient;     // GSMClient for TCP (mux 0)
extern AsyncA7670Client GSMClientSSL;  // GSMClientSSL for SSL base (mux 1)
extern AsyncA7670Client GSMClientSSL2; // GSMClientSSL2 for SSL base (mux 2)
extern AsyncA7670Client GSMFotaClient; // GSMFotaClient for FOTA non SSL (mux 2)
extern AsyncA7670Client GSMZigbeeClient; // Zigbee bridge client (mux 3)
#else
// Use TinyGSM library (default)
extern TinyGsm modem;
extern TinyGsmClient GSMClient;
extern AsyncA7670 modemAsync;
extern AsyncA7670Client GSMClientAsync;
#endif
extern WiFiClient wifiClient;
extern SSLClient GSMClient_SSL;
extern SSLClient GSMClient_SSL2;
extern SSLClient WifiClient_SSL;
extern SSLClient *tcpSslClient;
// extern ESP_SSLClient GSMClient_SSL;
// extern ESP_SSLClient WifiClient_SSL;
// extern ESP_SSLClient *tcpSslClient;
extern Client *tcpNonSslClient;
extern Client *zigbeeTcpClient;

// Configurator over BLE externs
extern NimBLEExtAdvertising *pAdvertising;      /** BLE Advertiser */
extern NimBLECharacteristic *pCharacteristicRX; /** Characteristic for Rx */
extern NimBLECharacteristic *pCharacteristicTX; /** Characteristic for Tx */
extern NimBLEServer *pServer;                   /** BLE Server */
extern NimBLEService *pService;                 /** BLE Service */

// BLE objects externs
extern NimBLEScan *pBLEScan;
// extern StaticJsonDocument<512> doc;
extern uint16_t connHandle;

// Legacy Preferences object - DEPRECATED, use UnifiedSettingsStorage instead
// extern Preferences settings;

// RGB Neopixel LED
extern Adafruit_NeoPixel rgbLed;
extern byte rgbBrightness;

extern uint16_t mtuNegotiatedSize;
// Note: bleDevicesMap replaced with SPIRAM array in BleMemoryManager

// Shared SPI bus initialization flag (camera + SD share same SPI lines)
extern bool spiSharedInitialized;
extern SemaphoreHandle_t spiBusMutex;
extern SPIClass SPI; // HSPI shared bus for SD + Arducam
#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
extern BlackBoxLayrzHub blackboxSDCARD; // BlackBox instance for SD card
#endif

// ------- Realtime queue (holds pointers to String) -------
static constexpr int RT_QUEUE_LEN = 128; // tune for your load
// QueueHandle_t         g_rtq           = nullptr;
// TaskHandle_t          g_netTask       = nullptr;

// ------- Fairness knobs -------
static constexpr int RT_BURST_MAX = 64; // max realtime msgs per tick
static constexpr int BB_BURST_MAX = 8;  // max backlog msgs per tick
static constexpr int RT_LOW_WATER = 8;  // if <= this, we allow backlog bursts

#endif // __GLOBALOBJECTSLAYRZHUB__

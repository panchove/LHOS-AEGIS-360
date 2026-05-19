#pragma once

#ifndef __DEFINITIONSLAYRZHUB_H__
#define __DEFINITIONSLAYRZHUB_H__

#include <driver/ledc.h>

#ifndef BUILD_NUMBER
#define BUILD_NUMBER 0
#endif
#define UARTSerial Serial
// #define USBSerial Serial
#define TINY_GSM_MODEM_A7670
#define TINY_GSM_DEBUG UARTSerial
#define SSL_DEBUG UARTSerial
#define FORMAT_LITTLEFS_IF_FAILED true
#define TIME_DRIFT_INFO
#ifndef USE_ASYNC_A7670
#define USE_ASYNC_A7670
#endif

// List of Service and Characteristic UUIDs
#define NUS_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define RX_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHUNK_SIZE 252

// GPIO pins
#define IO1 1 // GPIO1 pin in external terminal block J5.I/O-10 (IO1)
#define IO2 2 // GPIO2 pin in external terminal block J5.I/O-9  (IO2)
#define IO3 3 // GPIO3 pin in COMM module terminal block J6.COM-4 (K-POWER(IO3))
#define IO4 4 // GPIO4 pin in COMM module terminal block J6.COM-7 (SLEEP  (IO4))
#define IO5                                                                    \
  5 // GPIO5 pin in internal I/O terminal block J7.I/O-6 (IO5)(ALSO ACC INT2)
#define IO6                                                                    \
  6 // GPIO6 pin in internal I/O terminal block J7.I/O-5 (IO6)(ALSO ACC INT1)
#define IO7                                                                    \
  7 // GPIO7 pin in internal I/O terminal block J7.I/O-4 (IO7)(ALSO MAG INT)
#define IO19                                                                   \
  19 // GPIO19 pin in internal I/O terminal block J8.SPI-5 (IO19)(ALSO USB_DN)
#define IO20                                                                   \
  20 // GPIO20 pin in internal I/O terminal block J8.SPI-7 (IO20)(ALSO USB_DP)
#define IO41                                                                   \
  41 // GPIO41 pin in internal I/O terminal block J5.I/O-2 (IO41)(ALSO RS232_2
     // TX)
#define IO42                                                                   \
  42 // GPIO42 pin in internal I/O terminal block J5.I/O-1 (IO42)(ALSO RS232_2
     // RX)
#define IO14                                                                   \
  14 // GPIO14 pin in internal I/O terminal block J7.I/O-3 (IO14)(ALSO MAG DRDY)
#define IO46 46 // GPIO46 pin in internal I/O terminal block J7.I/O-2 (IO46)
#define IO47 47 // GPIO47 pin in internal I/O terminal block J7.I/O-1 (IO47)
#if defined(LAYRZ_HUB1_BUILD) || defined(LAYRZ_HUB2_BUILD)
#define I2C_SDA 8   // I2C SDA pin J9.I2C-2
#define I2C_SCL 9   // I2C SCL pin J9.I2C-1
#define CS 10       // Internal SPI CS pin J8.SPI-6
#define SPI_MOSI 11 // Internal SPI MOSI pin J8.SPI-3
#define SPI_SCK 12  // Internal SPI SCK pin J8.SPI-2
#define SPI_MISO 13 // Internal SPI MISO pin J8.SPI-1
#define SD_CS 14    // SD Card CS pin J8.SPI-8
#define ARDUCAM_CS 10
#define ARDUCAM_MOSI 11
#define ARDUCAM_MISO 13
#define ARDUCAM_SCK 12
#define IO45 45        // GPIO45 pin in internal I/O terminal block J8.SPI-4
#define USB_DN 19      // USB D- pin
#define USB_DP 20      // USB D+ pin
#define COMM_RX 15     // HUB Rx pin from COMM module Tx pin
#define COMM_TX 16     // HUB Tx pin to COMM module Rx pin
#define RS485_RX 18    // RS485 Rx pin
#define RS485_TX 17    // RS485 Tx pin
#define RS485_RE_DE 21 // RS485 RE_DE pin
#define RS232_1_RX 40  // RS232_1 Rx pin J5.I/O-5
#define RS232_1_TX 39  // RS232_1 Tx pin J5.I/O-6
#define RS232_2_RX 42  // RS232_2 Rx pin J5.I/O-3
#define RS232_2_TX 41  // RS232_2 Tx pin J5.I/O-4
#elif defined(LAYRZ_HUB25_BUILD)
#define DI1 4
#define DI2 5
#define DI3 6
#define DI4 42
#define AI1_DI5 7
#define AI2_DI6 8
#define DO1 45
#define DO2 46
#define VBAT_SENSE 3
#define I2C_SDA 9
#define I2C_SCL 10
#define CS 11
#define SPI_MOSI 12
#define SPI_SCK 13
#define SPI_MISO 14
#define SD_CS 41
#define EXT_CS 11
#define ARDUCAM_CS 11
#define ARDUCAM_MOSI 12
#define ARDUCAM_MISO 14
#define ARDUCAM_SCK 13
#define USB_DN 19
#define USB_DP 20
#define COMM_RX 40
#define COMM_TX 39
#define RS485_RX 18
#define RS485_TX 17
#define RS485_RE_DE 21
#define RS232_1_RX 16
#define RS232_1_TX 15
#define ONEWIRE 38
#define ACC_INT 47
#endif

#define LED 48       // pin number for RGB LED
#define ZIGBEE_RX 19 // Zigbee Rx pin from Zigbee module Tx pin
#define ZIGBEE_TX 20 // Zigbee Tx pin to Zigbee module Rx pin

// Layrz server definitions
#define LAYRZ_API_URL "api.layrz.com"
#define HTTPS_PORT 443
#define STABLE_SOCKET_PORT 9595
#define TESTING_SOCKET_PORT 9596
#define STABLE_SERVER_URL "link.layrz.network"
#define TESTING_SERVER_URL "next.link.layrz.network"
#define DATA_PATH "/v2/message"
#define COMMAND_PATH "/v2/commands"
#define NET_TRIALS 3
#define NET_CONNECTIVITY_REBOOT_TIMEOUT_MS 1800000UL // 30 minutes

// FOTA definitions
#define FOTA_SERVER "firmwares.layrz.com"
#define FOTA_PORT 80

// BUFFERS definitions

#define SETTINGS_BUFFER_SIZE 8192
#define BLE_DATA_SIZE 1024
#define MESSAGE_BUFFER_SIZE 8192
#define COMMAND_BUFFER_SIZE 1024
#define RESPONSE_BUFFER_SIZE 1024
#define UART_IO_BUFFER_SIZE 4096
#define RS485_BUFFER_SIZE 4096
#define RS232_1_BUFFER_SIZE 4096
#define RS232_2_BUFFER_SIZE 4096
#define EXT_IO_BUFFER_SIZE 8192
#define CANBUS_BUFFER_SIZE 16384
#define OBD2_DTC_BUFFER_SIZE 2048
#define MODBUS_MSG_BUFFER_SIZE 8192
#define MEDIA_MSG_BUFFER_SIZE 500000 // 500KB

// PWM definitions
#define PWM_FREQUENCY 10000              // 10 kHz
#define PWM_RESOLUTION LEDC_TIMER_12_BIT // 12-bit resolution (0-4095)
#define PWM_MAX_DUTY                                                           \
  ((1 << 12) - 1) // Maximum duty cycle value for 12-bit resolution

// CANBUS definitions

#define OBD2_STD_250K 0
#define OBD2_STD_500K 1
#define SAEJ1939_STD_250K 2
#define SAEJ1939_FMS_250K 3
#define SAEJ1939_STD_500K 4
#define SAEJ1939_FMS_500K 5

// ZIGBEE Coordinator definitions

#define UART_READ_CHUNK 8192
#define NET_READ_CHUNK 8192
#define UPLINK_RING_BYTES 2 * 1024 * 1024
#define HELLO_TIMEOUT_MS 5000
#define RECONNECT_MIN_MS 1000
#define RECONNECT_MAX_MS 15000
#define ZIGBEE_HUB_PORT 41609
// #define ZIGBEE_HUB_PORT 16186
#define ZB_CELL_CONNECT_TIMEOUT_MS 30000
#define ZB_CELL_STATUS_REFRESH_MS 5000

// Memory circuit breaker definitions
// If critical low-memory condition persists, device performs a controlled
// restart.
#define MEM_CB_ENABLE 1
#define MEM_CB_AUTO_RESTART 1
#define MEM_CB_BOOT_GRACE_MS 300000UL // 5 minutes after boot
#define MEM_CB_CONSECUTIVE_SAMPLES 2  // Number of consecutive low-memory checks
#define MEM_CB_MIN_HOLD_MS                                                     \
  10000UL // Low-memory condition must persist for at least 10s
#define MEM_CB_HEAP_INT_LOW_BYTES 12288UL // Free internal heap threshold
#define MEM_CB_MAX_INT_LOW_BYTES 6144UL // Largest internal free block threshold
#define MEM_CB_HEAP_INT_CRIT_BYTES                                             \
  8192UL // Critical free internal heap threshold
#define MEM_CB_MAX_INT_CRIT_BYTES                                              \
  2048UL // Critical largest internal free block threshold
#define MEM_CB_FRAG_INT_LOW_PCT 45UL // Internal heap fragmentation threshold
#define MEM_CB_HEAP_PS_LOW_BYTES 131072UL // Free PSRAM threshold (128KB)

// Keepalive ping period: send a minimal <Pd> if data_pub_per exceeds this value
// (seconds)
#define PING_PERIOD_SECS 240UL // 4 minutes

// Global health monitor definitions
#define SYS_HEALTH_MONITOR_PERIOD_MS 10000UL
#define SYS_HEALTH_AUTO_RESTART 1
#define SYS_HEALTH_STALL_SAMPLES 3
#define SYS_HEALTH_PD_STALL_FACTOR 4UL
#define SYS_HEALTH_PD_STALL_MIN_MS 180000UL
#define SYS_HEALTH_BLE_STALL_MS 90000UL

#endif // __DEFEXTERNS_H__

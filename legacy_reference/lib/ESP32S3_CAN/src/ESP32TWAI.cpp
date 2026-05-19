
#include "ESP32TWAI.h"

#include <Arduino.h>

ESP32TWAIClass::ESP32TWAIClass() {
  g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(TWAI_TX_PIN, TWAI_RX_PIN, TWAI_MODE_NORMAL);
  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
}

// Core CAN functionality
// bool ESP32TWAIClass::begin(long baudrate) {
//     if (!configureTWAI(baudrate)) {
//         return false;
//     }

//     if (twai_start() != ESP_OK) {
//         Serial.println("Failed to start TWAI driver");
//         return false;
//     }

//     Serial.println("TWAI started successfully");
//     return true;
// }

bool ESP32TWAIClass::begin(long baudrate) {
  if (!configureTWAI(baudrate)) {
    Serial.println("Initial TWAI configuration failed. Attempting recovery...");
    vTaskDelay(pdMS_TO_TICKS(500));
    twai_initiate_recovery();
    vTaskDelay(pdMS_TO_TICKS(500));

    if (!configureTWAI(baudrate)) {
      Serial.println("TWAI recovery failed. Check hardware.");
      return false;
    }
  }

  if (twai_start() != ESP_OK) {
    Serial.println("Failed to start TWAI driver, trying recovery...");
    twai_initiate_recovery();
    vTaskDelay(pdMS_TO_TICKS(500));

    if (twai_start() != ESP_OK) {
      Serial.println("TWAI could not start after recovery.");
      return false;
    }
  }

  Serial.println("TWAI started successfully");
  return true;
}

void ESP32TWAIClass::end() {
  twai_stop();
  twai_driver_uninstall();
  Serial.println("TWAI stopped");
}

int ESP32TWAIClass::available() {
  twai_status_info_t status_info;
  twai_get_status_info(&status_info);
  return status_info.msgs_to_rx;
}

CANMessage ESP32TWAIClass::receive() {
  twai_message_t rx_msg;
  CANMessage can_msg;

  if (twai_receive(&rx_msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
    can_msg.id = rx_msg.identifier;
    can_msg.length = rx_msg.data_length_code;
    memcpy(can_msg.data, rx_msg.data, rx_msg.data_length_code);
  }

  return can_msg;
}

bool ESP32TWAIClass::send(const CANMessage &message) {
  twai_message_t tx_msg;
  tx_msg.identifier = message.id;
  tx_msg.data_length_code = message.length;
  memcpy(tx_msg.data, message.data, message.length);
  tx_msg.flags = TWAI_MSG_FLAG_NONE;

  return twai_transmit(&tx_msg, pdMS_TO_TICKS(1000)) == ESP_OK;
}

// Packet management
int ESP32TWAIClass::beginPacket(int id, int dlc, bool rtr) {
  _txMsg.identifier = id;
  _txMsg.extd = 0; // Standard frame
  _txMsg.rtr = rtr ? 1 : 0;
  _txMsg.data_length_code = dlc;
  _txLength = 0; // Reset data length tracker
  return 1;
}

int ESP32TWAIClass::beginPacket(int id, int dlc) {
  return beginPacket(id, dlc, false);
}

int ESP32TWAIClass::beginExtendedPacket(long id, int dlc, bool rtr) {
  _txMsg.identifier = id;
  _txMsg.extd = 1; // Extended frame
  _txMsg.rtr = rtr ? 1 : 0;
  _txMsg.data_length_code = dlc;
  _txLength = 0; // Reset data length tracker
  return 1;
}

int ESP32TWAIClass::beginExtendedPacket(long id, int dlc) {
  return beginExtendedPacket(id, dlc, false);
}

int ESP32TWAIClass::endPacket() {
  if (_txLength > 8) {
    return 0; // CAN frames can't exceed 8 bytes
  }

  memcpy(_txMsg.data, _txData, _txLength);
  _txMsg.data_length_code = _txLength;

  return twai_transmit(&_txMsg, pdMS_TO_TICKS(1000)) == ESP_OK ? 1 : 0;
}

int ESP32TWAIClass::parsePacket() {
  twai_message_t rx_msg;
  if (twai_receive(&rx_msg, pdMS_TO_TICKS(100)) == ESP_OK) {
    _rxMsg = rx_msg;
    _rxLength = rx_msg.data_length_code;
    memcpy(_rxData, rx_msg.data, _rxLength);
    _rxIndex = 0; // Reset reading index
    return _rxLength;
  }
  return 0;
}

long ESP32TWAIClass::packetId() { return _rxMsg.identifier; }

bool ESP32TWAIClass::packetExtended() { return _rxMsg.extd; }

bool ESP32TWAIClass::packetRtr() { return _rxMsg.rtr; }

int ESP32TWAIClass::packetDlc() { return _rxMsg.data_length_code; }

// Data handling
size_t ESP32TWAIClass::write(uint8_t byte) {
  if (_txLength >= 8) {
    return 0;
  }
  _txData[_txLength++] = byte;
  return 1;
}

size_t ESP32TWAIClass::write(const uint8_t *buffer, size_t size) {
  size_t bytesToWrite = min(size, (size_t)(8 - _txLength));
  memcpy(_txData + _txLength, buffer, bytesToWrite);
  _txLength += bytesToWrite;
  return bytesToWrite;
}

int ESP32TWAIClass::read() {
  if (_rxIndex >= _rxLength) {
    return -1; // No more data
  }
  return _rxData[_rxIndex++];
}

size_t ESP32TWAIClass::readBytes(uint8_t *buffer, size_t length) {
  size_t bytesToRead = min(length, (size_t)(_rxLength - _rxIndex));
  memcpy(buffer, _rxData + _rxIndex, bytesToRead);
  _rxIndex += bytesToRead;
  return bytesToRead;
}

int ESP32TWAIClass::peek() {
  if (_rxIndex >= _rxLength) {
    return -1;
  }
  return _rxData[_rxIndex];
}

void ESP32TWAIClass::flush() {
  _rxIndex = _rxLength; // Skip remaining data
}

// Filtering
int ESP32TWAIClass::filter(int id, int mask) {
  f_config.acceptance_code = id << 21;
  f_config.acceptance_mask = mask << 21;
  f_config.single_filter = true;

  twai_stop();
  twai_driver_install(&g_config, &t_config, &f_config);
  twai_start();
  return 1;
}

int ESP32TWAIClass::filter(int id) {
  return filter(id, 0x7FF); // Standard 11-bit exact match
}

int ESP32TWAIClass::filterExtended(long id, long mask) {
  f_config.acceptance_code = id << 21;
  f_config.acceptance_mask = mask << 21;
  f_config.single_filter = true;

  twai_stop();
  twai_driver_install(&g_config, &t_config, &f_config);
  twai_start();
  return 1;
}

int ESP32TWAIClass::filterExtended(long id) {
  return filterExtended(id, 0x1FFFFFFF); // Extended 29-bit exact match
}

// Additional features
int ESP32TWAIClass::observe() {
  return twai_initiate_recovery() == ESP_OK ? 1 : 0;
}

int ESP32TWAIClass::loopback() {
  twai_stop();
  g_config.mode = TWAI_MODE_NO_ACK;
  twai_driver_install(&g_config, &t_config, &f_config);
  twai_start();
  return 1;
}

int ESP32TWAIClass::sleep() { return twai_stop() == ESP_OK ? 1 : 0; }

int ESP32TWAIClass::wakeup() { return twai_start() == ESP_OK ? 1 : 0; }

// Receive callback
void ESP32TWAIClass::onReceive(void (*callback)(int)) {
  _onReceive = callback;

  // Configure alerts for RX queue full and message received
  twai_reconfigure_alerts(TWAI_ALERT_RX_DATA | TWAI_ALERT_RX_QUEUE_FULL, NULL);

  // Create a task to monitor TWAI alerts (recommended for ESP32)
  xTaskCreatePinnedToCore(
    [](void *arg) {
      ESP32TWAIClass *can = static_cast<ESP32TWAIClass *>(arg);
      uint32_t alerts;

      while (true) {
        // Wait for an alert indefinitely
        if (twai_read_alerts(&alerts, portMAX_DELAY) == ESP_OK) {
          if (alerts & TWAI_ALERT_RX_DATA) {
            if (can->_onReceive) {
              can->_onReceive(can->available());
            }
          }
        }
      }
    },
    "TWAI_ISR_Task", 4096, this, 1, NULL, 1);
}

void IRAM_ATTR onTwaiInterrupt(void *arg) {
  ESP32TWAIClass *can = (ESP32TWAIClass *)arg;
  if (can->_onReceive) {
    can->_onReceive(can->available());
  }
}

// Internal helper functions
bool ESP32TWAIClass::configureTWAI(long baudrate) {
  switch (baudrate) {
  case 250000:
    t_config = TWAI_TIMING_CONFIG_250KBITS();
    break;
  case 500000:
    t_config = TWAI_TIMING_CONFIG_500KBITS();
    break;
  case 1000000:
    t_config = TWAI_TIMING_CONFIG_1MBITS();
    break;
  default:
    Serial.println("Unsupported baudrate");
    return false;
  }

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    // Serial.println("Failed to install TWAI driver");
    return false;
  }
  return true;
}

void ESP32TWAIClass::handleError() {
  twai_status_info_t status;
  twai_get_status_info(&status);

  // Check for Bus-Off error
  if (status.state == TWAI_STATE_BUS_OFF) {
    Serial.println("[ERROR] TWAI Bus-Off detected. Attempting recovery...");
    twai_initiate_recovery(); // Recover from bus-off state
    return;
  }

  // Error Warning: TX or RX error count >= 96
  if (status.tx_error_counter >= 96 || status.rx_error_counter >= 96) {
    Serial.println("[WARNING] TWAI is in Error Warning state.");
  }

  // Error Passive: TX or RX error count >= 128
  if (status.tx_error_counter >= 128 || status.rx_error_counter >= 128) {
    Serial.println("[WARNING] TWAI is in Error Passive state.");
  }

  // Check for FIFO overflow (missed messages)
  if (status.rx_missed_count > 0) {
    Serial.print("[ERROR] RX FIFO Overflow. Missed Frames: ");
    Serial.println(status.rx_missed_count);
  }

  // Arbitration lost monitoring
  if (status.arb_lost_count > 0) {
    Serial.print("[WARNING] Arbitration Lost Count: ");
    Serial.println(status.arb_lost_count);
  }

  // Display transmit errors
  if (status.tx_error_counter > 0) {
    Serial.print("[ERROR] TX Error Count: ");
    Serial.println(status.tx_error_counter);
  }

  // Display receive errors
  if (status.rx_error_counter > 0) {
    Serial.print("[ERROR] RX Error Count: ");
    Serial.println(status.rx_error_counter);
  }
}

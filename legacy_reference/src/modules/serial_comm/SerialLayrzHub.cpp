#include <esp_heap_caps.h>
#include <modules/serial_comm/SerialLayrzHub.h>

SemaphoreHandle_t rs485Semaphore = xSemaphoreCreateBinary();

void SerialMonitor::serialMonitorTask(void *pvParameters) {
  init();
  monitorPorts();
}

void SerialMonitor::init() {

  if (hubSettings.rs485_en) {
    SoftwareSerialConfig serialConfig;
    if (hubSettings.rs485_bits == 8) {
      if (hubSettings.rs485_par == "even") {
        serialConfig = SWSERIAL_8E1;
      } else if (hubSettings.rs485_par == "odd") {
        serialConfig = SWSERIAL_8O1;
      } else {
        serialConfig = SWSERIAL_8N1;
      }
    } else {
      if (hubSettings.rs485_par == "even") {
        serialConfig = SWSERIAL_7E1;
      } else if (hubSettings.rs485_par == "odd") {
        serialConfig = SWSERIAL_7O1;
      } else {
        serialConfig = SWSERIAL_7N1;
      }
    }

    RS485.begin(hubSettings.rs485_baud, serialConfig, RS485_RX, RS485_TX);
    pinMode(RS485_RE_DE, OUTPUT);
    digitalWrite(RS485_RE_DE, LOW);

    if (rs485Semaphore != nullptr) {
      xSemaphoreGive(rs485Semaphore); // Set semaphore to available
    }
    debugPrint("RS485 initialized\n");
  }

  if (hubSettings.rs232_1_en) {
    SoftwareSerialConfig serialConfig;
    if (hubSettings.rs232_1_bits == 8) {
      if (hubSettings.rs232_1_par == "even") {
        serialConfig = SWSERIAL_8E1;
      } else if (hubSettings.rs232_1_par == "odd") {
        serialConfig = SWSERIAL_8O1;
      } else {
        serialConfig = SWSERIAL_8N1;
      }
    } else {
      if (hubSettings.rs232_1_par == "even") {
        serialConfig = SWSERIAL_7E1;
      } else if (hubSettings.rs232_1_par == "odd") {
        serialConfig = SWSERIAL_7O1;
      } else {
        serialConfig = SWSERIAL_7N1;
      }
    }

    RS232_1.begin(hubSettings.rs232_1_baud, serialConfig, RS232_1_RX,
                  RS232_1_TX, false, 2048);
    debugPrint("RS232_1 initialized\n");
  }

#if !defined(LAYRZ_HUB25_BUILD)
  if (hubSettings.rs232_2_en) {
    SoftwareSerialConfig serialConfig;
    if (hubSettings.rs232_2_bits == 8) {
      if (hubSettings.rs232_2_par == "even") {
        serialConfig = SWSERIAL_8E1;
      } else if (hubSettings.rs232_2_par == "odd") {
        serialConfig = SWSERIAL_8O1;
      } else {
        serialConfig = SWSERIAL_8N1;
      }
    } else {
      if (hubSettings.rs232_2_par == "even") {
        serialConfig = SWSERIAL_7E1;
      } else if (hubSettings.rs232_2_par == "odd") {
        serialConfig = SWSERIAL_7O1;
      } else {
        serialConfig = SWSERIAL_7N1;
      }
    }

    RS232_2.begin(hubSettings.rs232_2_baud, serialConfig, RS232_2_RX,
                  RS232_2_TX, false, 2048);
    debugPrint("RS232_2 initialized\n");
  }
#endif

  if (hubSettings.uart_en) {
    SoftwareSerialConfig serialConfig;
    if (hubSettings.uart_bits == 8) {
      if (hubSettings.uart_par == "even") {
        serialConfig = SWSERIAL_8E1;
      } else if (hubSettings.uart_par == "odd") {
        serialConfig = SWSERIAL_8O1;
      } else {
        serialConfig = SWSERIAL_8N1;
      }
    } else {
      if (hubSettings.uart_par == "even") {
        serialConfig = SWSERIAL_7E1;
      } else if (hubSettings.uart_par == "odd") {
        serialConfig = SWSERIAL_7O1;
      } else {
        serialConfig = SWSERIAL_7N1;
      }
    }
#if defined(LAYRZ_HUB25_BUILD)
    hubSettings.uart_tx_pin = 20; // Override TX pin for LAYRZ_HUB25_BUILD
    hubSettings.uart_rx_pin = 19; // Override RX pin for LAYRZ_HUB25_BUILD
#endif

    UART_IO.begin(hubSettings.uart_baud, serialConfig, hubSettings.uart_rx_pin,
                  hubSettings.uart_tx_pin, false, 2048);
    debugPrint("UART initialized\n");
  }
}

void SerialMonitor::monitorPorts() {
  esp_task_wdt_add(serialMonitorHandle);
  for (;;) {
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
      if (hubSettings.uart_en && hubSettings.uart_mode == "transparent") {
        getTransData(UART_IO, "uart", UART_IO_BUFFER_SIZE,
                     hubSettings.uart_start, hubSettings.uart_end,
                     hubSettings.uart_buffer);
      }
      if (hubSettings.uart_en && hubSettings.uart_mode == "A01NYUB") {
        memset(uartIoDataBuffer, 0,
               UART_IO_BUFFER_SIZE); // initialize with zeros
        getA01nyub(UART_IO);
      }
      if (hubSettings.uart_en && hubSettings.uart_mode == "ioExtender") {
        getIoExtenderData(UART_IO);
      }
      if (hubSettings.rs485_en && hubSettings.rs485_mode == "transparent") {
        getTransData(RS485, "rs485", RS485_BUFFER_SIZE, hubSettings.rs485_start,
                     hubSettings.rs485_end, hubSettings.rs485_buffer);
      }
      if (hubSettings.rs485_en && hubSettings.rs485_mode == "HPC168") {
        memset(rs485DataBuffer, 0, RS485_BUFFER_SIZE); // initialize with zeros
        getHpc168Data(hubSettings.hpc_front_addr, hubSettings.hpc_back_addr,
                      hubSettings.hpc_back_en);
      }
      if (hubSettings.rs485_en && hubSettings.rs485_mode == "DN23E08") {
        memset(rs485DataBuffer, 0, RS485_BUFFER_SIZE); // initialize with zeros
        getDN23E08Data(hubSettings.dn23e08_s_addr, hubSettings.dn23e08_e_addr);
      }
      if (hubSettings.rs232_1_en &&
          (hubSettings.rs232_1_mode == "transparent" ||
           hubSettings.rs232_1_mode == "ri505_ticket_reader")) {
        getTransData(RS232_1, "rs232_1", RS232_1_BUFFER_SIZE,
                     hubSettings.rs232_1_start, hubSettings.rs232_1_end,
                     hubSettings.rs232_1_buffer);
      }
#if !defined(LAYRZ_HUB25_BUILD)
      if (hubSettings.rs232_2_en &&
          (hubSettings.rs232_2_mode == "transparent" ||
           hubSettings.rs232_2_mode == "ri505_ticket_reader")) {
        getTransData(RS232_2, "rs232_2", RS232_2_BUFFER_SIZE,
                     hubSettings.rs232_2_start, hubSettings.rs232_2_end,
                     hubSettings.rs232_2_buffer);
      }
#endif
    }
    xSemaphoreGive(xSemaphore);
    esp_task_wdt_reset();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

static inline int hexVal(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  return -1;
}

void SerialMonitor::getTransData(HardwareSerial &serialPort,
                                 const char *portName, uint16_t dataBufferSize,
                                 std::string startMarker, std::string endMarker,
                                 int bufferSize) {
  if (!isValidTime)
    return;
  std::string buffer = "";

  while (serialPort.available() && buffer.length() < bufferSize) {
    buffer += (char)serialPort.read();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  // if (buffer.length() > 0 && hubSettings.sys_debug_en) {
  //     UARTSerial.print("Buffer: ");
  //     for (int i = 0; i < buffer.length(); i++)
  //     {
  //         String hex = String(buffer[i], HEX);
  //         if (hex.length() < 2) hex = "0" + hex;
  //         UARTSerial.print(hex);
  //     }
  //     UARTSerial.println();
  // }

  while (serialPort.available()) {
    serialPort.read();
  }
  if (buffer.length() == 0)
    return;

  std::string dataPrefix = "";
  char *transBuffer = static_cast<char *>(
    heap_caps_malloc(dataBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!transBuffer) {
    debugPrint("Failed to allocate transBuffer in PSRAM\n");
    return;
  }
  memset(transBuffer, 0, dataBufferSize);
  if (startMarker.length() % 2 != 0) {
    startMarker.insert(0, "0");
  }
  if (endMarker.length() % 2 != 0) {
    endMarker.insert(0, "0");
  }
  // debugPrint("startMarker: %s\n", startMarker.c_str());
  // debugPrint("endMarker: %s\n", endMarker.c_str());

  if (!startMarker.empty() && !endMarker.empty() && buffer.length() > 0) {
    std::vector<uint8_t> startBytes = hexStringToBytes(startMarker);
    std::vector<uint8_t> endBytes = hexStringToBytes(endMarker);
    int startIndex = findBytes(buffer, startBytes);
    int endIndex = findBytes(buffer, endBytes, startIndex + startBytes.size());
    // debugPrint("startIndex: %d\n", startIndex);
    // debugPrint("endIndex: %d\n", endIndex);

    if (startIndex != -1 && endIndex != -1) {
      dataPrefix = std::string(portName) + std::string(".transparent.data:") +
                   stringToHexString(buffer.substr(
                     startIndex, endIndex - startIndex + endBytes.size()));
    }
  } else if (buffer.length() > 0) {
    dataPrefix = std::string(portName) + std::string(".transparent.data:") +
                 stringToHexString(buffer);
  }
  // debugPrint("dataPrefix: %s\n", dataPrefix.c_str());
  // strncat(dataBuffer, dataPrefix.c_str(), dataBufferSize - strlen(dataBuffer)
  // - 1);
  if (dataPrefix.length() > 0) {
    dataPrefix = std::to_string(time(nullptr)) + ";;;;;;;;report:LKSER," +
                 dataPrefix + ";";
    std::string crc = calculateCRC16(dataPrefix.c_str());
    size_t totalLen = dataPrefix.length() + crc.length() + 9; // <Pd> + </Pd>
    if (totalLen + 1 > dataBufferSize) {
      debugPrint("Serial transparent message too long (%u > %u), dropping\n",
                 static_cast<unsigned int>(totalLen + 1),
                 static_cast<unsigned int>(dataBufferSize));
      free(transBuffer);
      return;
    }
    strncat(transBuffer, dataPrefix.c_str(),
            dataBufferSize - strlen(transBuffer) - 1);
    strncat(transBuffer, crc.c_str(), dataBufferSize - strlen(transBuffer) - 1);
    for (int i = strlen(transBuffer); i >= 0; i--) {
      transBuffer[i + 4] = transBuffer[i]; // Shift characters to the right
    }
    transBuffer[0] = '<';
    transBuffer[1] = 'P';
    transBuffer[2] = 'd';
    transBuffer[3] = '>';
    strncat(transBuffer, "</Pd>", dataBufferSize - strlen(transBuffer) - 1);
    debugPrint("transBuffer length: %d\n", strlen(transBuffer));
    transBuffer[strlen(transBuffer)] = '\0';
    // debugPrint("Serial transparent message: ");
    // if (hubSettings.sys_debug_en) {
    //     for (int i = 0; i < strlen(transBuffer); i++)
    //     {
    //         UARTSerial.print(transBuffer[i]);
    //     }
    //     UARTSerial.println("");
    // }
    //
    BusMessage bm;
    bm.kind = BusMsgKind::PdSensor;
    bm.data = MessageBusLayrzHub::allocAndCopy(transBuffer, &bm.len);
    debugPrint("Publishing <Pd> from serial interface to MessageBus\n");
    bm.persistIfFail = true;
    bm.freeAfterSend = true;
    if (bm.data && isValidTime) {
      MessageBusLayrzHub::publish(bm);
    } else {
      debugPrint("Failed to alloc for Pd publish or not NTP synced\n");
    }
    free(transBuffer);
  }
}

void SerialMonitor::getTransData(SoftwareSerial &serialPort,
                                 const char *portName, uint16_t dataBufferSize,
                                 std::string startMarker, std::string endMarker,
                                 int bufferSize) {
  if (!isValidTime)
    return;
  std::string buffer = "";

  while (serialPort.available() && buffer.length() < bufferSize) {
    buffer += (char)serialPort.read();
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  if (buffer.length() > 0 && hubSettings.sys_debug_en) {
    debugPrint("Serial Buffer: ");
    for (int i = 0; i < buffer.length(); i++) {
      String hex = String(buffer[i], HEX);
      if (hex.length() < 2)
        hex = "0" + hex;
      if (Print *console = debugConsole())
        console->print(hex);
    }
    if (Print *console = debugConsole())
      console->println();
  }

  while (serialPort.available()) {
    serialPort.read();
  }
  if (buffer.length() == 0)
    return;

  std::string dataPrefix = "";
  char *transBuffer = static_cast<char *>(
    heap_caps_malloc(dataBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!transBuffer) {
    debugPrint("Failed to allocate transBuffer in PSRAM\n");
    return;
  }
  memset(transBuffer, 0, dataBufferSize);
  if (startMarker.length() % 2 != 0) {
    startMarker.insert(0, "0");
  }
  if (endMarker.length() % 2 != 0) {
    endMarker.insert(0, "0");
  }
  // debugPrint("startMarker: %s\n", startMarker.c_str());
  // debugPrint("endMarker: %s\n", endMarker.c_str());
  std::string hexStringData = "";
  if (!startMarker.empty() && !endMarker.empty() && buffer.length() > 0) {
    std::vector<uint8_t> startBytes = hexStringToBytes(startMarker);
    std::vector<uint8_t> endBytes = hexStringToBytes(endMarker);
    int startIndex = findBytes(buffer, startBytes);
    int endIndex = findBytes(buffer, endBytes, startIndex + startBytes.size());
    // debugPrint("startIndex: %d\n", startIndex);
    // debugPrint("endIndex: %d\n", endIndex);

    if (startIndex != -1 && endIndex != -1) {
      hexStringData = stringToHexString(
        buffer.substr(startIndex, endIndex - startIndex + endBytes.size()));
    }
  } else if (buffer.length() > 0) {
    hexStringData = stringToHexString(buffer);
  }
  if (hexStringData.length() == 0) {
    free(transBuffer);
    return;
  }
  dataPrefix =
    std::string(portName) + std::string(".transparent.data:") + hexStringData;
  if (hubSettings.rs232_1_mode == "ri505_ticket_reader" ||
      hubSettings.rs232_2_mode == "ri505_ticket_reader") {
    std::string ticketJson = calculate_json(hexStringData);
    if (ticketJson.length() > 0) {
      debugPrint("RI505 Ticket JSON: %s\n", ticketJson.c_str());
      // send ticket via http or https to RI API
      // url, token from settings (hubSettings.ri_api_url,
      // hubSettings.ri_api_token) POST url: hubSettings.ri_api_url Headers:
      // Authorization: FlespiToken <hubSettings.ri_api_token> if token not
      // empty Content-Type: application/json

      BusMessage bm;
      bm.kind = BusMsgKind::RiTicket;
      bm.data = MessageBusLayrzHub::allocAndCopy(ticketJson.c_str(), &bm.len);
      bm.persistIfFail = false;
      bm.freeAfterSend = true;
      if (bm.data) {
        MessageBusLayrzHub::publish(bm);
      } else {
        debugPrint("Failed to alloc for RI ticket publish\n");
      }
    }
  }
  // debugPrint("dataPrefix: %s\n", dataPrefix.c_str());
  // strncat(dataBuffer, dataPrefix.c_str(), dataBufferSize - strlen(dataBuffer)
  // - 1);
  if (dataPrefix.length() > 0) {
    dataPrefix = std::to_string(time(nullptr)) + ";;;;;;;;report:LKSER," +
                 dataPrefix + ";";
    std::string crc = calculateCRC16(dataPrefix.c_str());
    size_t totalLen = dataPrefix.length() + crc.length() + 9; // <Pd> + </Pd>
    if (totalLen + 1 > dataBufferSize) {
      debugPrint("Serial transparent message too long (%u > %u), dropping\n",
                 static_cast<unsigned int>(totalLen + 1),
                 static_cast<unsigned int>(dataBufferSize));
      free(transBuffer);
      return;
    }
    strncat(transBuffer, dataPrefix.c_str(),
            dataBufferSize - strlen(transBuffer) - 1);
    strncat(transBuffer, crc.c_str(), dataBufferSize - strlen(transBuffer) - 1);
    for (int i = strlen(transBuffer); i >= 0; i--) {
      transBuffer[i + 4] = transBuffer[i]; // Shift characters to the right
    }
    transBuffer[0] = '<';
    transBuffer[1] = 'P';
    transBuffer[2] = 'd';
    transBuffer[3] = '>';
    strncat(transBuffer, "</Pd>", dataBufferSize - strlen(transBuffer) - 1);
    debugPrint("transBuffer length: %d\n", strlen(transBuffer));
    transBuffer[strlen(transBuffer)] = '\0';
    // debugPrint("Serial transparent message: ");
    // if (hubSettings.sys_debug_en) {
    //     for (int i = 0; i < strlen(transBuffer); i++)
    //     {
    //         UARTSerial.print(transBuffer[i]);
    //     }
    //     UARTSerial.println("");
    // }
    // std::string netAuth = ("LayrzAuth " + getBluetoothMACAddress() +
    // ";").c_str(); if (isValidTime && isSocketAuth) {
    //     BaseClientLayrzHub* client = ClientFactoryLayrzHub::createClient();
    //     if (!client) {
    //         debugPrint("Error: Failed to create a Transport Client!\n");
    //         // vTaskDelete(NULL);
    //         return;
    //     }
    //     serverResponse cmdResponse = client->sendDataToServer(transBuffer);
    //     if (cmdResponse.responseCode != 200) debugPrint("Failed to send
    //     serial transparent data\n"); else debugPrint("Serial transparent data
    //     sent successfully\n"); delete client;
    // }
    BusMessage bm;
    bm.kind = BusMsgKind::PdSensor;
    bm.data = MessageBusLayrzHub::allocAndCopy(transBuffer, &bm.len);
    debugPrint("Publishing <Pd> from serial interface to MessageBus\n");
    bm.persistIfFail = true;
    bm.freeAfterSend = true;
    if (bm.data && isValidTime) {
      MessageBusLayrzHub::publish(bm);
    } else {
      debugPrint("Failed to alloc for Pd publish or not NTP synced\n");
    }
    free(transBuffer);
  }
}

void SerialMonitor::getA01nyub(SoftwareSerial &serialPort) {
  unsigned char data[4] = {};
  int distance;
  do {
    for (int i = 0; i < 4; i++) {
      data[i] = serialPort.read();
    }
  } while (data[0] != 0xff);

  serialPort.flush();

  if (data[0] == 0xff) {
    int sum;
    sum = (data[0] + data[1] + data[2]) & 0x00FF;
    if (sum == data[3]) {
      distance = (data[1] << 8) + data[2];
      if (distance > 280) {
        // debugPrint("distance=%dmm\n", distance);
        sprintf(uartIoDataBuffer, "uart.sensor.distance:%d", distance);
      } else {
        // debugPrint("Below the lower limit\n");
        sprintf(uartIoDataBuffer, "uart.sensor.distance:9999");
      }
    }
  }
}

std::vector<uint8_t> SerialMonitor::hexStringToBytes(const std::string &hex) {
  std::vector<uint8_t> bytes;
  for (size_t i = 0; i < hex.length(); i += 2) {
    std::string byteString =
      hex.substr(i, 2); // Use std::string's substr method
    uint8_t byte =
      static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
    bytes.push_back(byte);
  }
  return bytes;
}

std::string SerialMonitor::stringToHexString(const std::string &input) {
  std::ostringstream hexStream;
  hexStream << std::uppercase << std::hex;

  for (char c : input) {
    if (static_cast<uint8_t>(c) < 0x10) {
      hexStream << '0'; // Add leading zero if needed
    }
    hexStream << static_cast<int>(static_cast<uint8_t>(c));
  }

  return hexStream.str();
}

int SerialMonitor::findBytes(const std::string &buffer,
                             const std::vector<uint8_t> &pattern, int start) {
  if (buffer.length() < pattern.size())
    return -1;
  for (size_t i = start; i <= buffer.length() - pattern.size(); ++i) {
    bool found = true;
    for (size_t j = 0; j < pattern.size(); ++j) {
      if (static_cast<uint8_t>(buffer[i + j]) != pattern[j]) {
        found = false;
        break;
      }
    }
    if (found)
      return i;
  }
  return -1;
}

bool SerialMonitor::resetHpc168(uint16_t frontAddr, uint16_t backAddr,
                                bool backAvailable) {
  uint16_t ids[] = {frontAddr, backAddr};
  uint8_t command_reset = 0x12;
  uint8_t length_reset = 0x00;
  std::string command_frame = "";
  bool resetStatus = true;
  for (int i = 0; i < 2; i++) {
    command_frame =
      build_command_frame(ids[i], command_reset, length_reset, "");
    std::string response = send_hpc_command(RS485, command_frame);
    commandResponse cmdResponse = parse_response(response);
    if (response.length() > 0)
      resetStatus = resetStatus && cmdResponse.response_valid;
    if (!backAvailable && i == 0)
      break;
  }
  return resetStatus;
}

void SerialMonitor::getHpc168Data(uint16_t frontAddr, uint16_t backAddr,
                                  bool backAvailable) {
  uint16_t ids[] = {frontAddr, backAddr};
  uint8_t command_query = 0x13;
  uint8_t length_query = 0x00;
  uint8_t command_check_door_status = 0x66;
  uint8_t length_check_door_status = 0x06;
  std::string command_frame = "";
  uint32_t passCount = 0;
  std::string hpc168Data = "";
  for (int i = 0; i < 2; i++) {
    command_frame =
      build_command_frame(ids[i], command_query, length_query, "");
    std::string response = send_hpc_command(RS485, command_frame);
    commandResponse cmdResponse = parse_response(response);
    if (cmdResponse.response_valid && response.length() > 0) {
      // debugPrint("Door address: %d - Passengers in: %d\n", ids[i],
      // cmdResponse.passengers_in_value); debugPrint("Door address: %d -
      // Passengers out: %d\n", ids[i], cmdResponse.passengers_out_value);
      if (i == 1)
        hpc168Data += ","; // Add comma if back door is available
      hpc168Data += "passengers.door." + std::to_string(ids[i]) +
                    ".in:" + std::to_string(cmdResponse.passengers_in_value) +
                    "," + "passengers.door." + std::to_string(ids[i]) +
                    ".out:" + std::to_string(cmdResponse.passengers_out_value);
    }
    command_frame = build_command_frame(ids[i], command_check_door_status,
                                        length_check_door_status, "");
    response = send_hpc_command(RS485, command_frame);
    cmdResponse = parse_response(response);
    if (cmdResponse.response_valid && response.length() > 0) {
      // debugPrint("Door address: %d - is closed: %s\n", ids[i],
      // cmdResponse.door_closed == 0 ? "false" : "true");
      hpc168Data +=
        ",passengers.door." + std::to_string(ids[i]) + ".closed:" +
        std::string(cmdResponse.door_closed == 0 ? "false" : "true");
    }
    if (!backAvailable)
      break;
  }

  if (hpc168Data.length() > 0) {

    int totalIn = 0;
    int totalOut = 0;

    // Search for "passengers.in:"
    size_t pos = hpc168Data.find(".in:");
    while (pos != std::string::npos) {
      size_t start = pos + std::string(".in:").length();
      size_t end = hpc168Data.find(",", start);
      totalIn += std::stoi(hpc168Data.substr(
        start, end - start));             // Extract value and add to totalIn
      pos = hpc168Data.find(".in:", end); // Find the next "passengers.in:"
    }

    // Search for "passengers.out:"
    pos = hpc168Data.find(".out:");
    while (pos != std::string::npos) {
      size_t start = pos + std::string(".out:").length();
      size_t end = hpc168Data.find(",", start);
      totalOut += std::stoi(hpc168Data.substr(
        start, end - start));              // Extract value and add to totalOut
      pos = hpc168Data.find(".out:", end); // Find the next "passengers.out:"
    }

    // Calculate total passengers count
    hpc168Data += ",passengers.count:" + std::to_string(totalIn - totalOut);
    // debugPrint("HPC Payload: %s\n", hpc168Data.c_str());
  } else {
    debugPrint("No valid response from HPC168\n");
    hpc168Data = "passengers.error:true";
  }
  strncat(rs485DataBuffer, hpc168Data.c_str(),
          RS485_BUFFER_SIZE - strlen(rs485DataBuffer) - 1);
}

// Helper function to convert int to hex string with leading zeros
std::string SerialMonitor::int_to_hex(uint64_t value, size_t width) {
  std::stringstream ss;
  ss << std::uppercase << std::setfill('0') << std::setw(width) << std::hex
     << value;
  return ss.str();
}

// Function to calculate checksum
std::string SerialMonitor::calculate_checksum(const std::string &hex_chain) {
  uint32_t sum = 0;

  // Iterate over the hex string two characters at a time (each byte is 2 hex
  // characters)
  for (size_t i = 0; i < hex_chain.length(); i += 2) {
    // Convert the two hex characters to a byte
    std::string byte_str = hex_chain.substr(i, 2);
    uint8_t byte_value =
      strtol(byte_str.c_str(), nullptr, 16); // Convert hex string to byte
    sum += byte_value;                       // Add byte value to sum
  }

  // No-carry sum (mod 256)
  uint8_t checksum = sum & 0xFF;

  // Convert checksum back to a two-character uppercase hex string
  char checksum_str[3]; // Buffer for 2 hex digits + null terminator
  sprintf(checksum_str, "%02X", checksum); // Convert to hex string

  return std::string(checksum_str); // Return as std::string
}

// Function to build the command frame
std::string SerialMonitor::build_command_frame(uint16_t id, uint8_t command,
                                               uint8_t length,
                                               const std::string &data) {
  char stx = 0x02;
  char etx = 0x03;

  // Convert ID, command, length to ASCII hex
  std::string id_str = int_to_hex(id, 4);           // ID in 4 digits
  std::string command_str = int_to_hex(command, 2); // Command in 2 digits
  std::string length_str = int_to_hex(length, 2);   // Length in 2 digits

  // Build frame without CHK and ETX
  std::string frame = id_str + command_str + length_str + data;

  // Calculate checksum and append it
  std::string checksum = calculate_checksum(frame);
  frame = std::string(1, stx) + frame + checksum + std::string(1, etx);

  return frame;
}

// Function to control RS485 RE/DE pin
void SerialMonitor::rs485_set_mode(bool write_mode) {
  if (write_mode) {
    digitalWrite(RS485_RE_DE, HIGH); // Enable transmit mode (RE/DE HIGH)
  } else {
    digitalWrite(RS485_RE_DE, LOW); // Enable receive mode (RE/DE LOW)
  }
}

// Function to send the command and receive the response over RS485
std::string SerialMonitor::send_hpc_command(Stream &serial_port,
                                            const std::string &command_frame) {
  // Set RS485 to write mode
  rs485_set_mode(true);

  // Send the command frame
  serial_port.print(command_frame.c_str());

  // Short delay to ensure transmission is complete before switching to receive
  // mode
  vTaskDelay(
    15 /
    portTICK_PERIOD_MS); // Adjust the delay based on baud rate and bus speed

  // Set RS485 to read mode
  rs485_set_mode(false);

  // Read the response
  std::string response = "";
  unsigned long start_time = millis();
  while (millis() - start_time < 500) { // Timeout after 500 ms
    if (serial_port.available()) {
      response += serial_port.read();
    }
  }
  // debugPrint("Response: %s\n" , response.c_str());
  // remove the first character if it is not STX
  if (response.length() > 0 && response[0] != 0x02) {
    response = response.substr(1);
  }

  return response;
}

// Function to parse the response
SerialMonitor::commandResponse
SerialMonitor::parse_response(const std::string &response) {
  commandResponse cmdResponse;
  cmdResponse.response_frame = response;
  if (response.length() == 0) {
    debugPrint("Invalid response length.\n");
    cmdResponse.response_valid = false;
    return cmdResponse;
  }
  if (response[0] == 0x02 && response[response.length() - 1] == 0x03) {
    // Extract data between STX and ETX
    std::string buffer = response.substr(1, response.length() - 2);
    std::string chk_str = buffer.substr(buffer.length() - 2, 2);
    if (chk_str != calculate_checksum(buffer.substr(0, buffer.length() - 2))) {
      debugPrint("Checksum mismatch!\n");
      cmdResponse.response_valid = false;
      return cmdResponse;
    }
    std::string id_str = buffer.substr(0, 4);
    std::string cmd_str = buffer.substr(4, 2);
    std::string data_len_str = buffer.substr(6, 2);
    std::string passengers_in_str = "";
    std::string passengers_out_str = "";
    std::string door_data_str = "";
    bool door_closed = false;

    // Split the byte array
    if (cmd_str == "93") {
      // Serial.println("Query command received, parsing data.");
      passengers_in_str = buffer.substr(8, 8);
      passengers_out_str = buffer.substr(16, 8);
      cmdResponse.response_valid = true;
      cmdResponse.passengers_in_value =
        static_cast<uint32_t>(strtol(passengers_in_str.c_str(), nullptr, 16));
      cmdResponse.passengers_out_value =
        static_cast<uint32_t>(strtol(passengers_out_str.c_str(), nullptr, 16));
      return cmdResponse;
    } else if (cmd_str == "E6") {
      // Serial.println("Check door status command received.");
      door_data_str = buffer.substr(8, 12);
      uint64_t door_data_value =
        static_cast<uint64_t>(strtoull(door_data_str.c_str(), nullptr, 16));
      if (door_data_value == 1103823438081)
        cmdResponse.door_closed = false;
      else
        cmdResponse.door_closed = true;
      cmdResponse.response_valid = true;
      return cmdResponse;
    } else if (cmd_str == "92") {
      // Serial.println("Reset command received.");
      std::string reset_status = buffer.substr(8, 2);
      if (reset_status == "06")
        cmdResponse.response_valid = true;
      else
        cmdResponse.response_valid = false;
      return cmdResponse;
    } else {
      debugPrint("Invalid command received.\n");
      cmdResponse.response_valid = false;
      return cmdResponse;
    }
  } else {
    debugPrint("Invalid response format.\n");
    cmdResponse.response_valid = false;
    return cmdResponse;
  }
}

void SerialMonitor::getIoExtenderData(SoftwareSerial &serialPort) {
  // debugPrint("Getting IO Extender data\n");
  long initialTime = millis();
  std::string bufferStr = "";
  while (serialPort.available() && bufferStr.length() < 128) {
    bufferStr += (char)serialPort.read();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  // if (bufferStr.length() > 0 && hubSettings.sys_debug_en) {
  //     UARTSerial.print("Buffer: ");
  //     for (int i = 0; i < bufferStr.length(); i++)
  //     {
  //         String hex = String(bufferStr[i], HEX);
  //         if (hex.length() < 2) hex = "0" + hex;
  //         UARTSerial.print(hex);
  //     }
  //     UARTSerial.println();
  // }
  if (bufferStr.length() == 0) {
    debugPrint("IO extender buffer empty\n");
    return;
  }

  // std::string dataPrefix = "";

  std::vector<uint8_t> startBytes = hexStringToBytes("0223");
  std::vector<uint8_t> endBytes = hexStringToBytes("2304");
  int startIndex = findBytes(bufferStr, startBytes);
  int endIndex = findBytes(bufferStr, endBytes, startIndex + startBytes.size());
  // debugPrint("Start index: %d, End index: %d\n", startIndex, endIndex);

  if (startIndex != -1 && endIndex != -1) {
    bufferStr = bufferStr.substr(startIndex, endIndex - startIndex + 2);
    // debugPrint("Buffer String: %s\n", stringToHexString(bufferStr).c_str());
    endIndex = bufferStr.length() - 2;
    // debugPrint("New End index: %d\n", endIndex);

    std::string bufferStrCRC =
      stringToHexString(bufferStr.substr(endIndex - 2, 2));
    // debugPrint("Received CRC: %s\n", bufferStrCRC.c_str());

    // calculate CRC16
    std::string crc =
      calculateCRC16(bufferStr.substr(0, endIndex - 2).c_str(), endIndex - 2);
    // debugPrint("Calculated: %s\n", crc);
    if (bufferStrCRC != crc) {
      debugPrint("IO Extender CRC16 mismatch\n");
      return;
    }
    // debugPrint("Buffer String: %s\n", stringToHexString(bufferStr).c_str());
    uint32_t diBits = 0;
    uint8_t doBits = 0;
    bool digitalInputs[32] = {0};
    bool digitalOutputs[6] = {0};
    uint16_t aiValues[16] = {0};
    uint8_t aoValues[6] = {0};
    diBits |= ((uint32_t)bufferStr[4] & 0xFF)
              << 24; // Byte 4 (0xFF in the example)
    diBits |= ((uint32_t)bufferStr[5] & 0xFF)
              << 16; // Byte 5 (0xFF in the example)
    diBits |= ((uint32_t)bufferStr[6] & 0xFF)
              << 8;                            // Byte 6 (0xFF in the example)
    diBits |= ((uint32_t)bufferStr[7] & 0xFF); // Byte 7 (0xFF in the example)
    // UARTSerial.print("Digital inputs: ");
    // UARTSerial.println(diBits, HEX);
    for (int i = 0; i < 32; i++) {
      digitalInputs[i] = (diBits >> i) & 1;
    }
    doBits |= ((uint8_t)bufferStr[8] & 0xFF); // Byte 8 (0xFF in the example)
    // UARTSerial.print("Digital outputs: ");
    // UARTSerial.println(doBits, HEX);
    for (int i = 0; i < 6; i++) {
      digitalOutputs[i] = (doBits >> i) & 1;
    }
    // UARTSerial.print("Digital outputs: ");
    // for (int i = 0; i < 6; i++) {
    //     UARTSerial.print(digitalOutputs[i]);
    //     UARTSerial.print(" ");
    // }
    // UARTSerial.println();
    for (int i = 0; i < 16; i++) {
      uint16_t highByteValue = (uint16_t)bufferStr[9 + i * 2] & 0xFF;
      uint16_t lowByteValue = (uint16_t)bufferStr[10 + i * 2] & 0xFF;
      aiValues[i] = (highByteValue << 8) | lowByteValue;
    }
    // UARTSerial.print("Analog inputs: ");
    // for (int i = 0; i < 16; i++) {
    //     UARTSerial.print(aiValues[i]);
    //     UARTSerial.print(" ");
    // }
    // UARTSerial.println();
    for (int i = 0; i < 6; i++) {
      aoValues[i] = (uint8_t)bufferStr[41 + i] & 0xFF;
    }
    // UARTSerial.print("Analog outputs: ");
    // for (int i = 0; i < 6; i++) {
    //     UARTSerial.print(aoValues[i]);
    //     UARTSerial.print(" ");
    // }
    // UARTSerial.println();
    int diStart = 22;
    int doStart = 2;
    int aoStart = 8;
    memset(extIoDataBuffer, 0, EXT_IO_BUFFER_SIZE); // initialize with zeros
    for (int i = 0; i < 32; i++) {
      strncat(extIoDataBuffer,
              ("ext.di." + std::to_string(i + diStart) + ":" +
               std::to_string(digitalInputs[i]) + ",")
                .c_str(),
              EXT_IO_BUFFER_SIZE - strlen(extIoDataBuffer) - 1);
    }
    for (int i = 0; i < 6; i++) {
      strncat(extIoDataBuffer,
              ("ext.do." + std::to_string(i + doStart) + ":" +
               std::to_string(digitalOutputs[i]) + ",")
                .c_str(),
              EXT_IO_BUFFER_SIZE - strlen(extIoDataBuffer) - 1);
      // dataPrefix += "ext.digital.output." + std::to_string(i + doStart) + ":"
      // + std::to_string(digitalOutputs[i]) + ",";
    }
    for (int i = 0; i < 16; i++) {
      strncat(extIoDataBuffer,
              ("ext.ai." + std::to_string(i) + ":" +
               std::to_string(aiValues[i]) + ",")
                .c_str(),
              EXT_IO_BUFFER_SIZE - strlen(extIoDataBuffer) - 1);
      // dataPrefix += "ext.analog.input." + std::to_string(i) + ":" +
      // std::to_string(aiValues[i]) + ",";
    }
    for (int i = 0; i < 6; i++) {
      strncat(extIoDataBuffer,
              ("ext.ao." + std::to_string(i + aoStart) + ":" +
               std::to_string(aoValues[i]) + ",")
                .c_str(),
              EXT_IO_BUFFER_SIZE - strlen(extIoDataBuffer) - 1);
      // dataPrefix += "ext.analog.output." + std::to_string(i + aoStart) + ":"
      // + std::to_string(aoValues[i]) + ",";
    }

    // strncat(extIoDataBuffer, dataPrefix.c_str(), EXT_IO_BUFFER_SIZE -
    // strlen(extIoDataBuffer)
    // - 1);
    // remove the last comma
    extIoDataBuffer[strlen(extIoDataBuffer) - 1] = '\0';
    // print extIoDataBuffer
    //  if (hubSettings.sys_debug_en) {
    //      UARTSerial.print("IO Extender data: ");
    //      UARTSerial.println(extIoDataBuffer);
    //  }
    //  debugPrint("Time to parse external IO: %d\n", millis() - initialTime);
  }
}

std::string SerialMonitor::buildDN23E08Cmd(byte addr, byte cmd, int length,
                                           byte doByte, uint8_t *doDuration) {
  byte command_frame[length] = {0};
  command_frame[0] = 0x02; // STX
  command_frame[1] = 0x23; // DN23E08 ID
  command_frame[2] = addr; // Address
  command_frame[3] = cmd;  // Command
  if (cmd == 0x01 || cmd == 0x04 || cmd == 0x05 || cmd == 0x07) {
    // get CRC16 over the first 4 bytes
    std::string crc = calculateCRC16((char *)command_frame, 4);
    command_frame[4] =
      (char)strtol(crc.substr(0, 2).c_str(), nullptr, 16); // CRC16 high byte
    command_frame[5] =
      (char)strtol(crc.substr(2, 2).c_str(), nullptr, 16); // CRC16 low byte
    command_frame[6] = 0x23;                               // DN23E08 ID
    command_frame[7] = 0x04;                               // END
  } else if (cmd == 0x02) {
    command_frame[4] = doByte;         // Set the digital output byte
    command_frame[5] = doDuration[0];  // Set the duration for the first output
    command_frame[6] = doDuration[1];  // Set the duration for the second output
    command_frame[7] = doDuration[2];  // Set the duration for the third output
    command_frame[8] = doDuration[3];  // Set the duration for the fourth output
    command_frame[9] = doDuration[4];  // Set the duration for the fifth output
    command_frame[10] = doDuration[5]; // Set the duration for the sixth output
    command_frame[11] =
      doDuration[6]; // Set the duration for the seventh output
    command_frame[12] = doDuration[7]; // Set the duration for the eighth output
    // get CRC16 over the first 13 bytes
    std::string crc = calculateCRC16((char *)command_frame, 13);
    command_frame[13] =
      (char)strtol(crc.substr(0, 2).c_str(), nullptr, 16); // CRC16 high byte
    command_frame[14] =
      (char)strtol(crc.substr(2, 2).c_str(), nullptr, 16); // CRC16 low byte
    command_frame[15] = 0x23;                              // DN23E08 ID
    command_frame[16] = 0x04;                              // END
  }
  return std::string((char *)command_frame, sizeof(command_frame));
}

std::string SerialMonitor::buildDN23E08SetDoCmd(uint8_t addr, uint8_t doIndex,
                                                uint8_t doValue,
                                                uint8_t doDuration) {
  byte command_frame[11] = {0};
  command_frame[0] = 0x02;             // STX
  command_frame[1] = 0x23;             // DN23E08 ID
  command_frame[2] = addr;             // Address
  command_frame[3] = 0x09;             // Send single DO command
  command_frame[4] = (byte)doIndex;    // Command
  command_frame[5] = (byte)doValue;    // Command
  command_frame[6] = (byte)doDuration; // Command

  // get CRC16
  std::string crc = calculateCRC16((char *)command_frame, 7);
  command_frame[7] =
    (char)strtol(crc.substr(0, 2).c_str(), nullptr, 16); // CRC16 high byte
  command_frame[8] =
    (char)strtol(crc.substr(2, 2).c_str(), nullptr, 16); // CRC16 low byte
  command_frame[9] = 0x23;                               // DN23E08 ID
  command_frame[10] = 0x04;                              // END
  return std::string((char *)command_frame, sizeof(command_frame));
}

std::vector<std::string>
SerialMonitor::extractFrames(const std::string &buffer) {
  std::vector<std::string> messages;
  const std::string startSeq = std::string("\x02\x23", 2);
  const std::string endSeq = std::string("\x23\x04", 2);
  size_t pos = 0;

  while (true) {
    size_t start = buffer.find(startSeq, pos);
    if (start == std::string::npos)
      break;
    size_t end = buffer.find(endSeq, start);
    if (end == std::string::npos)
      break;

    messages.push_back(buffer.substr(start, end - start + endSeq.length()));
    pos = end + endSeq.length();
  }

  return messages;
}

bool DN23E08Frame::checkCRC(const std::string &msg) {
  if (msg.size() < 8)
    return false;
  std::string crcReceived =
    SerialMonitor::stringToHexString(msg.substr(msg.length() - 4, 2));
  std::string crcCalc =
    calculateCRC16(msg.substr(0, msg.length() - 4).c_str(), msg.length() - 4);
  return crcReceived == crcCalc;
}

DN23E08Frame DN23E08Frame::parse(const std::string &msg) {
  DN23E08Frame frame{};
  frame.validCRC = checkCRC(msg);
  if (!frame.validCRC)
    return frame;

  frame.address = (uint8_t)msg[2];
  frame.command = (uint8_t)msg[3];

  if (frame.command == 0x01) {
    uint8_t diByte = (uint8_t)msg[4];
    for (int i = 0; i < 8; ++i) {
      frame.digitalInputs[i] = (diByte >> i) & 0x01;
    }

    frame.triggerCount = ((uint8_t)msg[5] << 8) | (uint8_t)msg[6];

    uint8_t doByte = (uint8_t)msg[7];
    for (int i = 0; i < 8; ++i) {
      frame.digitalOutputs[i] = (doByte >> i) & 0x01;
    }

    for (int i = 0; i < 4; ++i) {
      frame.currents[i] =
        ((uint8_t)msg[8 + i * 2] << 8) | (uint8_t)msg[9 + i * 2];
      frame.voltages[i] =
        ((uint8_t)msg[16 + i * 2] << 8) | (uint8_t)msg[17 + i * 2];
    }
  }

  return frame;
}

void SerialMonitor::getDN23E08Data(uint8_t startAddr, uint8_t endAddr) {
  if (xSemaphoreTake(rs485Semaphore, 0) != pdTRUE) {
    debugPrint("Skipping getDN23E08Data due to active command\n");
    return;
  }
  if (startAddr > endAddr) {
    debugPrint("Invalid address range\n");
    return;
  }
  if (startAddr < 0x01 || endAddr > 0x1F) {
    debugPrint("Invalid address range\n");
    return;
  }

  for (uint8_t addr = startAddr; addr <= endAddr; ++addr) {
    std::string command_frame = buildDN23E08Cmd(addr, 0x01, 8);
    debugPrint("Command frame: %s\n", stringToHexString(command_frame).c_str());
    while (digitalRead(RS485_RE_DE) == HIGH) {
      vTaskDelay(1 /
                 portTICK_PERIOD_MS); // Wait for RS485 to be in receive mode
    }
    // Set RS485 to write mode
    rs485_set_mode(true);
    vTaskDelay(1 / portTICK_PERIOD_MS); // Wait for RS485 to be in write mode
    // Send the command frame
    for (int i = 0; i < command_frame.length(); i++) {
      RS485.write(command_frame[i]);
    }
    RS485.flush(); // Ensure all data is sent
    // Short delay to ensure transmission is complete before switching to
    // receive mode
    vTaskDelay(
      50 /
      portTICK_PERIOD_MS); // Adjust the delay based on baud rate and bus speed
    rs485_set_mode(false);
  }

  long initialTime = millis();
  std::string bufferStr = "";
  while (RS485.available()) {
    bufferStr += (char)RS485.read();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  if (bufferStr.length() > 0 && hubSettings.sys_debug_en) {
    if (Print *console = debugConsole())
      console->print("Buffer: ");
    for (int i = 0; i < bufferStr.length(); i++) {
      String hex = String(bufferStr[i], HEX);
      if (hex.length() < 2)
        hex = "0" + hex;
      if (Print *console = debugConsole())
        console->print(hex);
    }
    if (Print *console = debugConsole())
      console->println();
  }
  if (bufferStr.length() == 0) {
    xSemaphoreGive(rs485Semaphore); // Release semaphore if no data received
    return;
  }

  memset(extIoDataBuffer, 0, EXT_IO_BUFFER_SIZE); // initialize with zeros
  std::vector<std::string> rawFrames = extractFrames(bufferStr);
  for (const auto &raw : rawFrames) {
    DN23E08Frame frame = DN23E08Frame::parse(raw);
    ;

    if (!frame.validCRC) {
      debugPrint("CRC invalid for frame from addr %02X\n", frame.address);
      continue;
    }

    if (frame.command == 0x01) {

      std::string diPrefix =
        "dn23e08." + std::to_string(frame.address) + ".di.";
      std::string doPrefix =
        "dn23e08." + std::to_string(frame.address) + ".do.";
      std::string viPrefix =
        "dn23e08." + std::to_string(frame.address) + ".vi.";
      std::string ciPrefix =
        "dn23e08." + std::to_string(frame.address) + ".ci.";
      for (int i = 0; i < 8; i++) {
        strncat(extIoDataBuffer,
                (diPrefix + std::to_string(i + 1) + ":" +
                 std::to_string(frame.digitalInputs[i]) + ",")
                  .c_str(),
                EXT_IO_BUFFER_SIZE - strlen(extIoDataBuffer) - 1);
      }
      strncat(
        extIoDataBuffer,
        (diPrefix + "trigger.count:" + std::to_string(frame.triggerCount) + ",")
          .c_str(),
        EXT_IO_BUFFER_SIZE - strlen(extIoDataBuffer) - 1);

      for (int i = 0; i < 8; i++) {
        strncat(extIoDataBuffer,
                (doPrefix + std::to_string(i + 1) + ":" +
                 std::to_string(frame.digitalOutputs[i]) + ",")
                  .c_str(),
                EXT_IO_BUFFER_SIZE - strlen(extIoDataBuffer) - 1);
      }

      for (int i = 0; i < 4; i++) {
        strncat(extIoDataBuffer,
                (ciPrefix + std::to_string(i + 1) + ":" +
                 std::to_string(frame.currents[i]) + ",")
                  .c_str(),
                EXT_IO_BUFFER_SIZE - strlen(extIoDataBuffer) - 1);
      }

      for (int i = 0; i < 4; i++) {
        strncat(extIoDataBuffer,
                (viPrefix + std::to_string(i + 1) + ":" +
                 std::to_string(frame.voltages[i]) + ",")
                  .c_str(),
                EXT_IO_BUFFER_SIZE - strlen(extIoDataBuffer) - 1);
      }
      extIoDataBuffer[strlen(extIoDataBuffer) - 1] =
        '\0'; // remove the last comma
    } else if (frame.command == 0x05) {
      debugPrint("Trigger from device %02X\n", frame.address);
      if (hubSettings.acam_trig_src == 0) {
        for (int i = 0; i < hubSettings.acam_trig_cnt; i++) {
          if (!ArducamLayrzHub::takePicture()) {
            debugPrint("Error taking picture\n");
          }
          vTaskDelay(hubSettings.acam_trig_int * 1000 / portTICK_PERIOD_MS);
        }
      }
    } else {
      debugPrint("Unknown frame from %02X cmd %02X\n", frame.address,
                 frame.command);
    }
  }
  xSemaphoreGive(rs485Semaphore); // Release semaphore after processing
}

void SerialMonitor::setDN23E08DigitalOutput(uint8_t rs485Addr, uint8_t doIndex,
                                            uint8_t doValue,
                                            uint8_t doDuration) {

  const TickType_t timeoutTicks = 5000 / portTICK_PERIOD_MS; // 5 second timeout
  TickType_t startTick = xTaskGetTickCount();
  while (xSemaphoreTake(rs485Semaphore, 100 / portTICK_PERIOD_MS) != pdTRUE) {
    if ((xTaskGetTickCount() - startTick) > timeoutTicks) {
      debugPrint("Timeout waiting for RS485 semaphore\n");
      return;
    }
  }
  vTaskDelay(300 / portTICK_PERIOD_MS); // Wait after last monitor task
  if (rs485Addr < 1 || rs485Addr > 0x1F) {
    debugPrint("Invalid address range\n");
    xSemaphoreGive(rs485Semaphore); // Release semaphore if invalid address
    return;
  }
  std::string command_frame =
    buildDN23E08SetDoCmd(rs485Addr, doIndex, doValue, doDuration);
  debugPrint("Set Single DO Command frame: %s\n",
             stringToHexString(command_frame).c_str());
  while (digitalRead(RS485_RE_DE) == HIGH) {
    vTaskDelay(1 / portTICK_PERIOD_MS); // Wait for RS485 to be in receive mode
  }
  // Set RS485 to write mode
  rs485_set_mode(true);
  vTaskDelay(1 / portTICK_PERIOD_MS); // Wait for RS485 to be in write mode
  // Send the command frame
  for (int i = 0; i < command_frame.length(); i++) {
    RS485.write(command_frame[i]);
  }
  RS485.flush(); // Ensure all data is sent
  // Short delay to ensure transmission is complete before switching to receive
  // mode
  vTaskDelay(
    50 /
    portTICK_PERIOD_MS); // Adjust the delay based on baud rate and bus speed
  rs485_set_mode(false);
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait before monitor resumes
  xSemaphoreGive(rs485Semaphore);
}

void SerialMonitor::setDN23E08DigitalOutputs(uint8_t startAddr, uint8_t endAddr,
                                             byte doByte, uint8_t *doDuration) {

  const TickType_t timeoutTicks = 5000 / portTICK_PERIOD_MS; // 5 second timeout
  TickType_t startTick = xTaskGetTickCount();
  while (xSemaphoreTake(rs485Semaphore, 100 / portTICK_PERIOD_MS) != pdTRUE) {
    if ((xTaskGetTickCount() - startTick) > timeoutTicks) {
      debugPrint("Timeout waiting for RS485 semaphore\n");
      return;
    }
  }
  vTaskDelay(300 / portTICK_PERIOD_MS); // Wait after last monitor task
  if (startAddr > endAddr) {
    debugPrint("Invalid address range\n");
    xSemaphoreGive(rs485Semaphore); // Release semaphore if invalid address
    return;
  }
  if (startAddr < 0x01 || endAddr > 0x1F) {
    debugPrint("Invalid address range\n");
    xSemaphoreGive(rs485Semaphore); // Release semaphore if invalid address
    return;
  }
  for (uint8_t addr = startAddr; addr <= endAddr; ++addr) {
    std::string command_frame =
      buildDN23E08Cmd(addr, 0x02, 17, doByte, doDuration);
    debugPrint("Set Multiple DO's Command frame: %s\n",
               stringToHexString(command_frame).c_str());
    while (digitalRead(RS485_RE_DE) == HIGH) {
      vTaskDelay(1 /
                 portTICK_PERIOD_MS); // Wait for RS485 to be in receive mode
    }
    // Set RS485 to write mode
    rs485_set_mode(true);
    vTaskDelay(1 / portTICK_PERIOD_MS); // Wait for RS485 to be in write mode
    // Send the command frame
    for (int i = 0; i < command_frame.length(); i++) {
      RS485.write(command_frame[i]);
    }
    RS485.flush(); // Ensure all data is sent

    // Short delay to ensure transmission is complete before switching to
    // receive mode
    vTaskDelay(
      50 /
      portTICK_PERIOD_MS); // Adjust the delay based on baud rate and bus speed
    rs485_set_mode(false);
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait before monitor resumes
  xSemaphoreGive(rs485Semaphore);
}

void SerialMonitor::rebootDN23E08(uint8_t startAddr, uint8_t endAddr) {
  const TickType_t timeoutTicks = 5000 / portTICK_PERIOD_MS; // 5 second timeout
  TickType_t startTick = xTaskGetTickCount();
  while (xSemaphoreTake(rs485Semaphore, 100 / portTICK_PERIOD_MS) != pdTRUE) {
    if ((xTaskGetTickCount() - startTick) > timeoutTicks) {
      debugPrint("Timeout waiting for RS485 semaphore\n");
      return;
    }
  }
  vTaskDelay(300 / portTICK_PERIOD_MS); // Wait after last monitor task
  if (startAddr > endAddr) {
    debugPrint("Invalid address range\n");
    xSemaphoreGive(rs485Semaphore); // Release semaphore if invalid address
    return;
  }
  if (startAddr < 0x01 || endAddr > 0x1F) {
    debugPrint("Invalid address range\n");
    xSemaphoreGive(rs485Semaphore); // Release semaphore if invalid address
    return;
  }
  for (uint8_t addr = startAddr; addr <= endAddr; ++addr) {
    std::string command_frame = buildDN23E08Cmd(addr, 0x04, 8);
    debugPrint("Reboot Command frame: %s\n",
               stringToHexString(command_frame).c_str());
    while (digitalRead(RS485_RE_DE) == HIGH) {
      vTaskDelay(1 /
                 portTICK_PERIOD_MS); // Wait for RS485 to be in receive mode
    }
    // Set RS485 to write mode
    rs485_set_mode(true);
    vTaskDelay(1 / portTICK_PERIOD_MS); // Wait for RS485 to be in write mode
    // Send the command frame
    for (int i = 0; i < command_frame.length(); i++) {
      RS485.write(command_frame[i]);
    }
    RS485.flush(); // Ensure all data is sent
    // Short delay to ensure transmission is complete before switching to
    // receive mode
    vTaskDelay(
      50 /
      portTICK_PERIOD_MS); // Adjust the delay based on baud rate and bus speed
    rs485_set_mode(false);
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait before monitor resumes
  xSemaphoreGive(rs485Semaphore);
}

void SerialMonitor::resetDN23E08TriggerCounter(uint8_t startAddr,
                                               uint8_t endAddr) {
  const TickType_t timeoutTicks = 5000 / portTICK_PERIOD_MS; // 5 second timeout
  TickType_t startTick = xTaskGetTickCount();
  while (xSemaphoreTake(rs485Semaphore, 100 / portTICK_PERIOD_MS) != pdTRUE) {
    if ((xTaskGetTickCount() - startTick) > timeoutTicks) {
      debugPrint("Timeout waiting for RS485 semaphore\n");
      return;
    }
  }
  vTaskDelay(300 / portTICK_PERIOD_MS); // Wait after last monitor task
  if (startAddr > endAddr) {
    debugPrint("Invalid address range\n");
    xSemaphoreGive(rs485Semaphore); // Release semaphore if invalid address
    return;
  }
  if (startAddr < 0x01 || endAddr > 0x1F) {
    debugPrint("Invalid address range\n");
    xSemaphoreGive(rs485Semaphore); // Release semaphore if invalid address
    return;
  }
  for (uint8_t addr = startAddr; addr <= endAddr; ++addr) {
    std::string command_frame = buildDN23E08Cmd(addr, 0x07, 8);
    debugPrint("Reset Counter Command frame: %s\n",
               stringToHexString(command_frame).c_str());
    while (digitalRead(RS485_RE_DE) == HIGH) {
      vTaskDelay(1 /
                 portTICK_PERIOD_MS); // Wait for RS485 to be in receive mode
    }
    // Set RS485 to write mode
    rs485_set_mode(true);
    vTaskDelay(1 / portTICK_PERIOD_MS); // Wait for RS485 to be in write mode
    // Send the command frame
    for (int i = 0; i < command_frame.length(); i++) {
      RS485.write(command_frame[i]);
    }
    RS485.flush(); // Ensure all data is sent
    // Short delay to ensure transmission is complete before switching to
    // receive mode
    vTaskDelay(
      50 /
      portTICK_PERIOD_MS); // Adjust the delay based on baud rate and bus speed
    rs485_set_mode(false);
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait before monitor resumes
  xSemaphoreGive(rs485Semaphore);
}

int SerialMonitor::setExternalOutput(uint8_t pin, uint8_t value, uint8_t mode) {
  if (mode == 1) {
    if (pin < 2 || pin > 7)
      return -1;
    if (value < 0 || value > 1)
      return -1;
  } else if (mode == 2) {
    if (pin < 8 || pin > 13)
      return -1;
    if (value < 0 || value > 255)
      return -1;
  }

  // std::string commandPayload = "";
  char *commandPayload = static_cast<char *>(
    heap_caps_malloc(10, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!commandPayload) {
    debugPrint("Failed to allocate commandPayload in PSRAM\n");
    return -1;
  }
  commandPayload[0] = (char)0x02;  // Start of frame
  commandPayload[1] = (char)0x23;  // Start of frame
  commandPayload[2] = (char)mode;  // mode (1 for DO,  2 for AO)
  commandPayload[3] = (char)pin;   // AO pin number
  commandPayload[4] = (char)value; // AO value
  // calculate CRC16
  std::string crcHexStr = calculateCRC16(commandPayload, 5);
  std::vector<uint8_t> crcBytes = hexStringToBytes(crcHexStr);
  commandPayload[5] = (char)crcBytes[0]; // CRC16 byte 1
  commandPayload[6] = (char)crcBytes[1]; // CRC16 byte 2
  commandPayload[7] = (char)0x04;        // End of frame
  commandPayload[8] = (char)0x23;        // End of frame
  commandPayload[9] = (char)0x00;        // Null terminator
  while (UART_IO.available()) {
    UART_IO.read();
  }
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  debugPrint("Command payload: ");
  for (int i = 0; i < 9; i++) {
    UART_IO.print(commandPayload[i]);
    if (hubSettings.sys_debug_en) {
      String hex = String(commandPayload[i], HEX);
      if (hex.length() < 2)
        hex = "0" + hex;
      if (Print *console = debugConsole())
        console->print(hex);
    }
  }
  if (hubSettings.sys_debug_en) {
    if (Print *console = debugConsole())
      console->println();
  }
  UART_IO.println();
  UART_IO.flush();
  free(commandPayload);

  std::string response = "";
  long initTime = millis();
  while (millis() - initTime < 1000) {
    while (UART_IO.available() && response.length() < 64) {
      response += (char)UART_IO.read();
      vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    if (response.length() > 2)
      break;
  }

  if (response.length() > 0 && hubSettings.sys_debug_en) {
    if (Print *console = debugConsole())
      console->print("Response: ");
    for (int i = 0; i < response.length(); i++) {
      String hex = String(response[i], HEX);
      if (hex.length() < 2)
        hex = "0" + hex;
      if (Print *console = debugConsole())
        console->print(hex);
    }
    if (Print *console = debugConsole())
      console->println();
  }

  if (response.length() == 0) {
    return 1;
  }
  std::vector<uint8_t> ackBytes = hexStringToBytes("060d0a");
  std::vector<uint8_t> nakBytes = hexStringToBytes("150d0a");
  int ackIndex = findBytes(response, ackBytes);
  int nakIndex = findBytes(response, nakBytes);
  debugPrint("Ack index: %d, Nak index: %d\n", ackIndex, nakIndex);
  if (ackIndex != -1) {
    response = response.substr(ackIndex, 3);
    debugPrint("ack response: %s\n", stringToHexString(response).c_str());
    return 0;
  } else if (nakIndex != -1) {
    response = response.substr(nakIndex, 3);
    debugPrint("nak response: %s\n", stringToHexString(response).c_str());
    return 2;
  }
  debugPrint("no response: %s\n", stringToHexString(response).c_str());
  return 3; // No response
}

bool SerialMonitor::rebootIOExtender() {
  std::string commandPayload = "";
  commandPayload += (char)0x02; // Start of frame
  commandPayload += (char)0x23; // Start of frame
  commandPayload += (char)0x03; // Reboot cmd
  commandPayload += (char)0x00; // Length
  commandPayload += (char)0x00; // Length
  // calculate CRC16
  std::string crcHexStr = calculateCRC16(commandPayload.c_str(), 5);
  std::vector<uint8_t> crcBytes = hexStringToBytes(crcHexStr);
  commandPayload += (char)crcBytes[0]; // CRC16 byte 1
  commandPayload += (char)crcBytes[1]; // CRC16 byte 2
  commandPayload += (char)0x04;        // End of frame
  commandPayload += (char)0x23;        // End of frame
  while (UART_IO.available()) {
    UART_IO.read();
  }
  vTaskDelay(100 / portTICK_PERIOD_MS);
  debugPrint("Command payload: ");
  for (int i = 0; i < commandPayload.length(); i++) {
    UART_IO.print(commandPayload[i]);
    if (hubSettings.sys_debug_en) {
      String hex = String(commandPayload[i], HEX);
      if (hex.length() < 2)
        hex = "0" + hex;
      if (Print *console = debugConsole())
        console->print(hex);
    }
  }
  if (hubSettings.sys_debug_en) {
    if (Print *console = debugConsole())
      console->println();
  }
  UART_IO.println();

  uint8_t *response = new uint8_t[10];
  int responseLength = 0;
  memset(response, 0, 10);
  long initTime = millis();
  while (millis() - initTime < 1000) {
    if (UART_IO.available()) {
      responseLength = UART_IO.readBytesUntil(0x0d, response, 10);
      break;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  debugPrint("elapsed time: %d\n", millis() - initTime);
  // for (int i = 0; i < responseLength; i++) {
  //     debugPrint("response[%d]: %02x\n", i, response[i]);
  // }
  debugPrint("Response: %s\n",
             stringToHexString(std::string((char *)response)).c_str());
  debugPrint("Response[0]: %02x\n", response[0]);

  if (!response) {
    delete[] response;
    return false;
  }
  if (response[0] == 0x06) {
    delete[] response;
    return true;
  }

  delete[] response;
  return false; // CRC error
}

// functions used by ri505_ticket rs232 parser
// ------------------------- Helpers -------------------------
std::vector<uint8_t> SerialMonitor::hexToBytes(const std::string &hex) {
  std::vector<uint8_t> out;
  out.reserve(hex.size() / 2);
  int hi = -1;
  for (char c : hex) {
    int v = hexVal(c);
    if (v < 0)
      continue;
    if (hi < 0)
      hi = v;
    else {
      out.push_back(static_cast<uint8_t>((hi << 4) | v));
      hi = -1;
    }
  }
  return out;
}

std::string SerialMonitor::trim(const std::string &s) {
  size_t i = 0, j = s.size();
  while (i < j && std::isspace(static_cast<unsigned char>(s[i])))
    i++;
  while (j > i && std::isspace(static_cast<unsigned char>(s[j - 1])))
    j--;
  return s.substr(i, j - i);
}

std::string SerialMonitor::collapseSpaces(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  bool inSpace = false;
  for (char ch : s) {
    if (ch == ' ') {
      if (!inSpace)
        out.push_back(' ');
      inSpace = true;
    } else {
      out.push_back(ch);
      inSpace = false;
    }
  }
  return out;
}

std::string SerialMonitor::toUpperCopy(const std::string &s) {
  std::string u = s;
  for (char &c : u)
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return u;
}

std::string SerialMonitor::jsonEscape(const std::string &s) {
  std::string o;
  o.reserve(s.size() + 16);
  for (unsigned char c : s) {
    switch (c) {
    case '\\':
      o += "\\\\";
      break;
    case '"':
      o += "\\\"";
      break;
    case '\b':
      o += "\\b";
      break;
    case '\f':
      o += "\\f";
      break;
    case '\n':
      o += "\\n";
      break;
    case '\r':
      o += "\\r";
      break;
    case '\t':
      o += "\\t";
      break;
    default:
      if (c < 0x20) {
        const char *hx = "0123456789ABCDEF";
        o += "\\u00";
        o.push_back(hx[(c >> 4) & 0xF]);
        o.push_back(hx[c & 0xF]);
      } else {
        o.push_back(static_cast<char>(c));
      }
    }
  }
  return o;
}

std::string SerialMonitor::normalizeNumber(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (c == ',')
      out.push_back('.');
    else if ((c >= '0' && c <= '9') || c == '.')
      out.push_back(c);
  }
  return out;
}

// ------------------------- ESC/POS stripping -------------------------

std::vector<uint8_t>
SerialMonitor::stripEscPos(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size());

  for (size_t i = 0; i < in.size();) {
    uint8_t b = in[i];

    if (b == 0x1B && i + 2 < in.size()) { // ESC
      uint8_t cmd = in[i + 1];
      if (cmd == 0x21) {
        i += 3;
        continue;
      } // ESC ! n
      if (cmd == 0x61) {
        i += 3;
        continue;
      } // ESC a n
      if (cmd == 0x64) {
        i += 3;
        continue;
      } // ESC d n
      i += 2;
      continue;
    }

    if (b == 0x1D && i + 2 < in.size()) { // GS
      uint8_t cmd = in[i + 1];
      if (cmd == 0x61) {
        i += 3;
        continue;
      } // GS a n
      if (cmd == 0x21) {
        i += 3;
        continue;
      } // GS ! n
      i += 2;
      continue;
    }

    if (b == 0x10 && i + 2 < in.size() && in[i + 1] == 0x05) { // DLE EOT n
      i += 3;
      continue;
    }

    if (b == 0x1F && i + 2 < in.size()) { // US ...
      i += 3;
      continue;
    }

    out.push_back(b);
    i += 1;
  }

  return out;
}

std::string SerialMonitor::bytesToCleanText(const std::vector<uint8_t> &data) {
  std::string cleaned;
  cleaned.reserve(data.size());
  for (uint8_t b : data) {
    if (b == 0x09)
      cleaned.append("    ");
    else if (b == 0x0A || b == 0x0D || (b >= 0x20 && b <= 0x7E))
      cleaned.push_back(static_cast<char>(b));
  }
  return cleaned;
}

std::string
SerialMonitor::collapseSpacesPerLinePreserveNL(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  bool inSpace = false;
  for (char c : s) {
    if (c == '\r' || c == '\n') {
      inSpace = false;
      out.push_back(c);
      continue;
    }
    if (c == ' ') {
      if (!inSpace)
        out.push_back(' ');
      inSpace = true;
    } else {
      out.push_back(c);
      inSpace = false;
    }
  }
  return out;
}

std::vector<std::string>
SerialMonitor::splitAndNormalizeLines(const std::string &text) {
  std::vector<std::string> lines;
  std::string line;
  line.reserve(64);

  for (size_t i = 0; i <= text.size(); ++i) {
    char c = (i < text.size()) ? text[i] : '\n';
    if (c == '\r')
      continue;
    if (c == '\n') {
      std::string t = trim(collapseSpaces(line));
      if (!t.empty())
        lines.push_back(t);
      line.clear();
    } else {
      line.push_back(c);
    }
  }
  return lines;
}

// ------------------------- Pattern match helpers -------------------------

bool SerialMonitor::looksLikeDate_ddmmyy(const std::string &s) {
  // dd/mm/yy or dd/mm/yyyy
  if (s.size() < 8)
    return false;
  auto isd = [&](size_t i) {
    return i < s.size() && std::isdigit((unsigned char)s[i]);
  };
  if (!isd(0) || !isd(1) || s[2] != '/' || !isd(3) || !isd(4) || s[5] != '/')
    return false;
  if (!isd(6) || !isd(7))
    return false;
  return true;
}

bool SerialMonitor::looksLikeTime_hhmmss(const std::string &s) {
  // hh:mm or hh:mm:ss
  if (s.size() < 5)
    return false;
  auto isd = [&](size_t i) {
    return i < s.size() && std::isdigit((unsigned char)s[i]);
  };
  if (!isd(0) || !isd(1) || s[2] != ':' || !isd(3) || !isd(4))
    return false;
  if (s.size() >= 8) {
    if (s[5] != ':' || !isd(6) || !isd(7))
      return false;
  }
  return true;
}

std::string SerialMonitor::parseNumberAtEnd(const std::string &line) {
  size_t end = line.size();
  while (end > 0 && std::isspace((unsigned char)line[end - 1]))
    end--;
  if (end == 0)
    return "";

  auto ok = [](char c) {
    return (c >= '0' && c <= '9') || c == '.' || c == ',';
  };
  size_t start = end;
  while (start > 0 && ok(line[start - 1]))
    start--;
  if (start == end)
    return "";
  return trim(line.substr(start, end - start));
}

std::string SerialMonitor::parseFieldLine(const std::string &line,
                                          const std::string &keyUpper) {
  // Accept:
  //   KEY: value
  //   KEY value
  //   KEY    value
  std::string up = toUpperCopy(line);
  size_t pos = up.find(keyUpper);
  if (pos == std::string::npos)
    return "";

  // ensure "word boundary" before key
  if (pos > 0 && std::isalnum((unsigned char)up[pos - 1]))
    return "";

  size_t i = pos + keyUpper.size();
  while (i < line.size() && line[i] == ' ')
    i++;
  if (i < line.size() && line[i] == ':')
    i++;
  while (i < line.size() && line[i] == ' ')
    i++;

  return trim(line.substr(i));
}

std::string SerialMonitor::parseTokenField(const std::string &line,
                                           const std::string &keyUpper) {
  // KEY: TOKEN or KEY TOKEN (token alnum only)
  std::string rest = parseFieldLine(line, keyUpper);
  if (rest.empty())
    return "";
  size_t j = 0;
  while (j < rest.size() && std::isalnum((unsigned char)rest[j]))
    j++;
  return trim(rest.substr(0, j));
}

std::string SerialMonitor::parseTotalSmart(const std::string &line) {
  std::string up = toUpperCopy(line);
  if (up.find("SUBTOTAL") != std::string::npos)
    return "";
  if (up.find("TOTAL") == std::string::npos)
    return "";
  // Prefer number at end (most robust)
  return parseNumberAtEnd(line);
}

// ------------------------- Main -------------------------

std::string SerialMonitor::calculate_json(const std::string &raw_ticket_hex) {
  if (raw_ticket_hex.empty())
    return "";
  // const std::string hex = "411d6100"; // Example hex string for testing"
  auto bytes = hexToBytes(raw_ticket_hex);
  auto stripped = stripEscPos(bytes);

  std::string text = bytesToCleanText(stripped);
  text = collapseSpacesPerLinePreserveNL(text);

  // NOTE: We avoid aggressive "repeat reduction" to not destroy numbers.
  // If you still want it, apply only to non-digits with a safe function.

  auto lines = splitAndNormalizeLines(text);
  // print lines for debug
  //  debugPrint("Ticket lines:\n");
  //  for (const auto& l : lines) {
  //      UARTSerial.printf("  %s\n", l.c_str());
  //  }
  //  UARTSerial.println();

  std::string AT, FOLIO, CLIENTE, NOMBRE, DOMICILIO, COLONIA;
  std::string fecha, hora;
  std::string LITROS_GLP, PRECIO_LTR, TOTAL;

  for (const auto &l : lines) {
    std::string up = toUpperCopy(l);

    if (AT.empty()) {
      std::string v = parseTokenField(l, "AT");
      if (!v.empty())
        AT = v;
    }
    if (FOLIO.empty()) {
      std::string v = parseTokenField(l, "FOLIO");
      if (!v.empty())
        FOLIO = v;
    }
    if (CLIENTE.empty()) {
      std::string v = parseFieldLine(l, "CLIENTE");
      if (!v.empty())
        CLIENTE = v;
    }
    if (NOMBRE.empty()) {
      std::string v = parseFieldLine(l, "NOMBRE");
      if (!v.empty())
        NOMBRE = v;
    }
    if (DOMICILIO.empty()) {
      std::string v = parseFieldLine(l, "DOMICILIO");
      if (!v.empty())
        DOMICILIO = v;
    }
    if (COLONIA.empty()) {
      std::string v = parseFieldLine(l, "COLONIA");
      if (!v.empty())
        COLONIA = v;
    }

    if (fecha.empty() && looksLikeDate_ddmmyy(l))
      fecha = l;
    if (hora.empty() && looksLikeTime_hhmmss(l))
      hora = l;

    // LITROS GLP: tolerate extra spaces
    if (LITROS_GLP.empty() && up.find("LITROS") != std::string::npos &&
        up.find("GLP") != std::string::npos) {
      LITROS_GLP = parseNumberAtEnd(l);
    }
    // PRECIO LTR
    if (PRECIO_LTR.empty() && up.find("PRECIO") != std::string::npos &&
        up.find("LTR") != std::string::npos) {
      PRECIO_LTR = parseNumberAtEnd(l);
    }
    // TOTAL
    if (TOTAL.empty()) {
      std::string t = parseTotalSmart(l);
      if (!t.empty())
        TOTAL = t;
    }
  }

  std::string FECHA_HORA;
  if (!fecha.empty() && !hora.empty())
    FECHA_HORA = fecha + " " + hora;
  else if (!fecha.empty())
    FECHA_HORA = fecha;
  else if (!hora.empty())
    FECHA_HORA = hora;

  std::string folio_n = normalizeNumber(FOLIO);
  std::string litros_n = normalizeNumber(LITROS_GLP);
  std::string precio_n = normalizeNumber(PRECIO_LTR);
  std::string total_n = normalizeNumber(TOTAL);
  std::string ident = hubSettings.ri_api_ident;
  std::string permit = hubSettings.ri_api_permit;

  std::string json = "{";
  json += "\"ident\":\"" + jsonEscape(ident) + "\",";
  json += "\"permiso\":\"" + jsonEscape(permit) + "\",";
  // json += "\"folio\":"      + (folio_n.empty()  ? "0" : folio_n)  + ",";
  // json += "\"cliente\":\""    + jsonEscape(CLIENTE)    + "\",";
  // json += "\"nombre\":\""     + jsonEscape(NOMBRE)     + "\",";
  // json += "\"domicilio\":\""  + jsonEscape(DOMICILIO)  + "\",";
  // json += "\"colonia\":\""    + jsonEscape(COLONIA)    + "\",";
  json += "\"fecha.hora\":\"" + jsonEscape(FECHA_HORA) + "\",";
  json += "\"litros.glp\":" + (litros_n.empty() ? "0" : litros_n);
  // json += "\"precio.ltr\":" + (precio_n.empty() ? "0" : precio_n) + ",";
  // json += "\"total\":"      + (total_n.empty()  ? "0" : total_n);
  json += "}";
  if (LITROS_GLP.empty()) {
    return "";
  }
  return json;
}

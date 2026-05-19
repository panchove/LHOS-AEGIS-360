#include "ARD1939ESP32S3.h"

#include <stdlib.h>
#include <string.h>

extern byte canInit(int baudRate);
extern byte canCheckError(void);
extern byte canTransmit(long, unsigned char *, int);
extern byte canReceive(long *, unsigned char *, int *);

// Define the global transport protocol message and timer
TransportMessage currentTPMessage = {};
TransportProtocolState tpState = {};
Timer transportTimeoutTimer;

// Initialize the J1939 protocol
byte ARD1939ESP32S3::Init(int systemTimeMs, int baudRate) {
  // Reset protocol state
  protocolState = {false,       NULLADDRESS, false, NULLADDRESS, NULLADDRESS,
                   NULLADDRESS, NULLADDRESS, false, false};

  // Initialize timers
  addressClaimTimer = {250 / systemTimeMs, true, false};
  retryTimer = {100 / systemTimeMs, true, false};

  // Reset message filters
  for (auto &filter : messageFilters) {
    filter.active = false;
    filter.pgn = 0;
  }

  // Initialize CAN interface
  // return CAN.begin(250000) ? 0 : 1; // 0 = success, 1 = failure
  return canInit(baudRate);
}

// Terminate the protocol
void ARD1939ESP32S3::Terminate() {
  Init(SYSTEM_TIME, 250000); // Reinitialize the protocol
}

// Set preferred address
void ARD1939ESP32S3::SetPreferredAddress(byte address) {
  protocolState.preferredAddress = address;
}

// Set address range
void ARD1939ESP32S3::SetAddressRange(byte start, byte end) {
  protocolState.addressRangeStart = start;
  protocolState.addressRangeEnd = end;
}

// Set the NAME field
void ARD1939ESP32S3::SetNAME(long identityNumber, int manufacturerCode,
                             byte functionInstance, byte ecuInstance,
                             byte function, byte vehicleSystem,
                             byte vehicleSystemInstance, byte industryGroup,
                             byte arbitraryAddressCapable) {
  nodeName[0] = (byte)(identityNumber & 0xFF);
  nodeName[1] = (byte)((identityNumber >> 8) & 0xFF);
  nodeName[2] =
    (byte)(((manufacturerCode << 5) & 0xFF) | (identityNumber >> 16));
  nodeName[3] = (byte)(manufacturerCode >> 3);
  nodeName[4] = (byte)((functionInstance << 3) | ecuInstance);
  nodeName[5] = function;
  nodeName[6] = (byte)(vehicleSystem << 1);
  nodeName[7] = (byte)((arbitraryAddressCapable << 7) | (industryGroup << 4) |
                       vehicleSystemInstance);
}

// Operate on received messages
// byte ARD1939ESP32S3::Operate(byte* messageType, long* pgn, byte* data, int*
// length, byte* destAddr, byte* srcAddr, byte* priority) {
//     byte status = ParseReceivedMessage(pgn, data, length, destAddr, srcAddr,
//     priority);

//     if (status == J1939_MSG_PROTOCOL) {
//         if (*pgn == 0x00EC00 || *pgn == 0x00EB00) { // TP.CM or TP.DT
//             return HandleTransportProtocol(*srcAddr, *pgn, *length);
//         }
//     }

//     return status;
// }

byte ARD1939ESP32S3::Operate(byte *messageType, uint32_t *pgn, byte *data,
                             int *length, byte *destAddr, byte *srcAddr,
                             byte *priority) {
  long rawId;

  // Receive a CAN message
  if (canReceive(&rawId, data, length) != 0) {
    return J1939_MSG_NONE; // No message available
  }

  // Extract J1939 fields from the CAN ID
  *priority = (rawId >> 26) & 0x7; // Priority (bits 26–28)
  *pgn = (rawId >> 8) & 0x03FFFF;  // PGN (bits 8–25)
  *srcAddr = rawId & 0xFF;         // Source Address (bits 0–7)

  if (*pgn >= 0xF000) {
    *destAddr = GLOBALADDRESS; // Global destination for PDU2 format
  } else {
    *destAddr = *pgn & 0xFF; // Destination address for PDU1 format
    *pgn &= 0xFF00;          // Mask out destination address bits
  }

  // Check for Transport Protocol (BAM or DT)
  if (*pgn == 0x00EC00 || *pgn == 0x00EB00) {
    // Handle Transport Protocol messages
    byte result = HandleTransportProtocol(data, *pgn, *srcAddr);

    if (result == J1939_MSG_APP) {
      // If a complete application message is assembled, return it
      memcpy(data, tpState.dataBuffer, tpState.messageSize);
      *length = tpState.messageSize;
      *pgn = tpState.pgn; // Update PGN correctly from TP state
      *messageType = J1939_MSG_APP;
      Serial.printf("Received complete message with PGN: %u\n", *pgn);
      Serial.printf("messageSize: %d\n", *length);

      // Reset transport protocol state
      tpState = {};
      return J1939_MSG_APP;
    }

    return result; // Return the TP result (e.g., BAM or protocol in progress)
  }

  // Non-Transport Protocol message
  *messageType = J1939_MSG_APP;
  return J1939_MSG_APP;
}

// Transmit a message
byte ARD1939ESP32S3::Transmit(byte priority, uint32_t pgn, byte srcAddr,
                              byte destAddr, byte *data, int length) {
  if (length > MAX_DATA_LENGTH) {
    return 1; // Error: Message too long
  }

  long canId = ((long)priority << 26) | (pgn << 8) | srcAddr;
  if (destAddr != GLOBALADDRESS) {
    canId |= ((long)destAddr << 8);
  }
  return canTransmit(canId, data, length); // 0 = success, 1 = failure
}

// Parse a received CAN message into J1939 fields
byte ARD1939ESP32S3::ParseReceivedMessage(uint32_t *pgn, byte *data,
                                          int *length, byte *destAddr,
                                          byte *srcAddr, byte *priority) {
  long rawId;

  // Receive the message
  if (canReceive(&rawId, data, length) != 0) {
    return J1939_MSG_NONE; // No message available
  }

  // Extract Priority (bits 26–28)
  *priority = (rawId >> 26) & 0x7;

  // Extract PGN (bits 8–25)
  long extractedPgn = (rawId >> 8) & 0x03FFFF;

  // Extract Source Address (bits 0–7)
  *srcAddr = rawId & 0xFF;

  // Determine Destination Address and finalize PGN
  if (extractedPgn >= 0xF000) {      // PDU2 format
    *destAddr = GLOBALADDRESS;       // Global destination
    *pgn = extractedPgn;             // Full PGN (18 bits)
  } else {                           // PDU1 format
    *destAddr = extractedPgn & 0xFF; // Destination address (lower byte of PGN)
    *pgn = extractedPgn & 0xFF00;    // Mask destination address bits
  }

  // Return message type
  return IsProtocolMessage(*pgn, data) ? J1939_MSG_PROTOCOL : J1939_MSG_APP;
}

// Check if the message is a protocol message (e.g., Address Claim)
bool ARD1939ESP32S3::IsProtocolMessage(uint32_t pgn, byte *data) {
  return (pgn == 0x00EA00 || pgn == 0x00EC00 ||
          pgn == 0x00EB00); // Common PGNs for J1939 protocol
}

// Handle Address Claim process
byte ARD1939ESP32S3::HandleAddressClaim(byte srcAddr, byte *incomingName) {
  if (protocolState.addressClaimFailed) {
    return ADDRESSCLAIM_FAILED;
  }
  if (protocolState.addressClaimSuccessful) {
    return ADDRESSCLAIM_FINISHED;
  }

  if (!protocolState.inProgress) {
    if (!AllocateNextAddress()) {
      protocolState.addressClaimFailed = true;
      return ADDRESSCLAIM_FAILED;
    }

    // Send an Address Claim message
    Transmit(6, 0x00EE00, protocolState.claimedAddress, GLOBALADDRESS, nodeName,
             MAX_NAME_LENGTH);
    addressClaimTimer = {250, true, false}; // Reset timer
    protocolState.inProgress = true;
  } else if (addressClaimTimer.hasExpired) {
    protocolState.addressClaimFailed = true;
    return ADDRESSCLAIM_FAILED;
  }

  return ADDRESSCLAIM_INPROGRESS;
}

// Allocate the next available address
bool ARD1939ESP32S3::AllocateNextAddress() {
  if (!protocolState.preferredAddressAvailable) {
    protocolState.claimedAddress = protocolState.preferredAddress;
    protocolState.preferredAddressAvailable = true;
    return true;
  }

  if (protocolState.claimedAddress < protocolState.addressRangeEnd) {
    protocolState.claimedAddress++;
    return true;
  }

  return false; // No more addresses available
}

// Compare two J1939 NAME fields to determine priority
byte ARD1939ESP32S3::CompareNAMEs(byte *incomingName, byte *localName) {
  for (int i = MAX_NAME_LENGTH - 1; i >= 0; --i) {
    if (incomingName[i] != localName[i]) {
      return (incomingName[i] < localName[i])
               ? 1
               : 2; // 1 = incoming wins, 2 = local wins
    }
  }
  return 0; // Names are equal
}

byte ARD1939ESP32S3::HandleTransportProtocol(byte *data, uint32_t pgn,
                                             byte srcAddr) {
  if (pgn == 0x00EC00) { // TP.CM BAM
    return HandleBAM(data, srcAddr);
  } else if (pgn == 0x00EB00) { // TP.DT
    return HandleDT(data, srcAddr);
  }
  return J1939_MSG_NONE;
}

byte ARD1939ESP32S3::HandleBAM(byte *data, byte srcAddr) {
  // // Validate Control Byte (BAM must have 0x20 as the first byte)
  // if (data[0] != 0x20) {
  //     Serial.println("Invalid BAM Control Byte.");
  //     return J1939_MSG_NONE;
  // }

  // Extract BAM parameters
  uint16_t messageSize = ((uint16_t)data[2] << 8) | data[1];
  uint8_t numPackets = data[3];
  uint32_t largePgn =
    ((uint32_t)data[7] << 16) | ((uint32_t)data[6] << 8) | data[5];

  // Initialize state
  tpState.pgn = largePgn;
  tpState.srcAddr = srcAddr;
  tpState.messageSize = messageSize;
  tpState.numPackets = numPackets;
  tpState.receivedPackets = 0;
  memset(tpState.dataBuffer, 0, sizeof(tpState.dataBuffer));
  memset(tpState.packetReceived, 0, sizeof(tpState.packetReceived));
  tpState.timeout = millis() + T1_TIMEOUT_MS;
  tpState.inProgress = true;

  Serial.printf("BAM Received. PGN: %06X, Size: %d, Packets: %d\n", largePgn,
                messageSize, numPackets);
  return J1939_MSG_PROTOCOL;
}

byte ARD1939ESP32S3::HandleDT(byte *data, byte srcAddr) {
  if (!tpState.inProgress || srcAddr != tpState.srcAddr) {
    Serial.println("Unexpected TP.DT packet or no active BAM.");
    return J1939_MSG_NONE;
  }

  // Extract sequence number and payload
  uint8_t sequenceNumber = data[0];
  int offset = (sequenceNumber - 1) * 7;
  int copyLength = min(7, tpState.messageSize - offset);

  // Validate sequence number
  if (sequenceNumber < 1 || sequenceNumber > tpState.numPackets) {
    Serial.printf("Invalid sequence number: %d\n", sequenceNumber);
    return J1939_MSG_NONE;
  }

  // Copy the payload into the buffer
  memcpy(&tpState.dataBuffer[offset], &data[1], copyLength);
  tpState.packetReceived[sequenceNumber - 1] = true;
  tpState.receivedPackets++;

  Serial.printf("TP.DT Packet Received. Seq: %d, Offset: %d, Length: %d\n",
                sequenceNumber, offset, copyLength);

  // Check if all packets are received
  if (tpState.receivedPackets == tpState.numPackets) {
    Serial.println("All TP.DT packets received. Assembling complete message.");
    tpState.inProgress = false;

    // Process the assembled message
    ProcessAssembledMessage(tpState.pgn, tpState.dataBuffer,
                            tpState.messageSize);

    return J1939_MSG_APP; // Indicate a complete application message
  }

  // Reset timeout for the next packet
  tpState.timeout = millis() + T3_TIMEOUT_MS;
  return J1939_MSG_PROTOCOL;
}

void ARD1939ESP32S3::CheckTimeouts() {
  if (tpState.inProgress && millis() > tpState.timeout) {
    Serial.println("Timeout occurred. Discarding incomplete TP message.");
    tpState = {}; // Reset the state
  }
}

void ARD1939ESP32S3::ProcessAssembledMessage(uint32_t pgn, byte *data,
                                             uint16_t length) {
  Serial.printf("Complete Message Assembled. PGN: %06X, Length: %d\n", pgn,
                length);
  for (int i = 0; i < length; i++) {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();
}

byte ARD1939ESP32S3::HandleTransportProtocolBAM(byte srcAddr, const byte *data,
                                                int length) {
  // Validate Control Byte
  // if (data[0] != 0x20) {
  //     Serial.println("Invalid BAM Control Byte.");
  //     return J1939_MSG_NONE;
  // }

  // Extract Message Length
  uint16_t messageSize = ((uint16_t)data[2] << 8) | data[1];

  // Extract Number of Packets
  uint8_t numPackets = data[3];

  // Reserved Byte
  if (data[4] != 0xFF) {
    Serial.println("Invalid Reserved Byte in BAM.");
    return J1939_MSG_NONE;
  }

  // Extract PGN
  uint32_t largePgn =
    ((uint32_t)data[7] << 16) | ((uint32_t)data[6] << 8) | data[5];

  // Validate message size and packet count
  if (messageSize > MAX_TRANSPORT_LENGTH ||
      numPackets > MAX_TRANSPORT_PACKETS) {
    Serial.println("BAM parameters out of bounds.");
    return J1939_MSG_NONE; // Discard invalid BAM
  }

  // Initialize transport protocol state
  currentTPMessage.pgn = largePgn;
  currentTPMessage.srcAddr = srcAddr;
  currentTPMessage.messageSize = messageSize;
  currentTPMessage.numPackets = numPackets;
  currentTPMessage.receivedPackets = 0;
  memset(currentTPMessage.dataBuffer, 0, sizeof(currentTPMessage.dataBuffer));
  memset(currentTPMessage.packetReceived, 0,
         sizeof(currentTPMessage.packetReceived));

  // Set timeout for the first data packet
  transportTimeoutTimer.start(T1_TIMEOUT_MS);

  Serial.printf("BAM Received. PGN: %06X, Size: %d, Packets: %d\n", largePgn,
                messageSize, numPackets);

  return J1939_MSG_PROTOCOL; // Indicate BAM was processed
}

byte ARD1939ESP32S3::HandleTransportProtocolDT(byte srcAddr, byte *data,
                                               int *length, uint32_t *pgn) {
  // Use a temporary variable to store the raw CAN ID
  long rawId;

  unsigned long startTime = millis(); // Start timeout timer

  while (currentTPMessage.receivedPackets < currentTPMessage.numPackets) {
    // Wait for the next packet
    if (canReceive(&rawId, data, length) ==
        0) { // Assuming canReceive() fetches the next packet
      // Extract the source address from rawId (bits 0–7 of the CAN ID)
      byte receivedSrcAddr = rawId & 0xFF;

      // Validate source address
      if (receivedSrcAddr != currentTPMessage.srcAddr) {
        Serial.println("Unexpected source address for TP.DT.");
        return J1939_MSG_NONE; // Ignore
      }

      // Extract the sequence number from the first byte
      uint8_t sequenceNumber = data[0];

      // Validate sequence number
      if (sequenceNumber < 1 || sequenceNumber > currentTPMessage.numPackets) {
        Serial.printf("Invalid sequence number: %d\n", sequenceNumber);
        return J1939_MSG_NONE;
      }

      // Copy the payload into the correct buffer position
      int offset = (sequenceNumber - 1) * 7;
      int copyLength = min(7, currentTPMessage.messageSize - offset);

      // Validate buffer boundaries
      if (offset + copyLength > sizeof(currentTPMessage.dataBuffer)) {
        Serial.printf(
          "Error: Buffer overflow. Offset: %d, CopyLen: %d, BufferSize: %d\n",
          offset, copyLength, sizeof(currentTPMessage.dataBuffer));
        return J1939_MSG_NONE;
      }

      // Store the packet's data
      memcpy(&currentTPMessage.dataBuffer[offset], &data[1], copyLength);
      currentTPMessage.packetReceived[sequenceNumber - 1] = true;
      currentTPMessage.receivedPackets++;

      Serial.printf("TP.DT Packet Received. Seq: %d, Offset: %d, Length: %d\n",
                    sequenceNumber, offset, copyLength);

      // Reset the timeout timer for the next packet
      startTime = millis();
    }

    // Check for timeout
    if (millis() - startTime > T3_TIMEOUT_MS) {
      Serial.println("Timeout waiting for TP.DT packets.");
      return J1939_MSG_NONE; // Timeout occurred
    }
  }

  // All packets received
  Serial.println("All TP.DT packets received. Assembling complete message.");

  // Process the complete large message
  *pgn = currentTPMessage.pgn;
  *length = currentTPMessage.messageSize;
  memcpy(data, currentTPMessage.dataBuffer, currentTPMessage.messageSize);

  // Reset transport protocol state
  currentTPMessage = {};

  return J1939_MSG_APP; // Indicate a complete application message
}

// Set a message filter for a specific PGN
byte ARD1939ESP32S3::SetMessageFilter(uint32_t pgn) {
  for (auto &filter : messageFilters) {
    if (!filter.active) {
      filter.active = true;
      filter.pgn = pgn;
      return 0; // Success
    }
  }
  return 1; // Failure: No available filter slots
}

// Delete a message filter
void ARD1939ESP32S3::DeleteMessageFilter(uint32_t pgn) {
  for (auto &filter : messageFilters) {
    if (filter.active && filter.pgn == pgn) {
      filter.active = false;
      filter.pgn = 0;
      break;
    }
  }
}

// Reset all timers
void ARD1939ESP32S3::ResetTimers() {
  InitializeTimer(&addressClaimTimer);
  InitializeTimer(&retryTimer);
}

// Initialize a specific timer
void ARD1939ESP32S3::InitializeTimer(TimerState *timer) {
  timer->countdown = 0;
  timer->isActive = false;
  timer->hasExpired = false;
}

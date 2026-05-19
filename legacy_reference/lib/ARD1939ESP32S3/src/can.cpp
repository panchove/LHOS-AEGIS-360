// ------------------------------------------------------------------------
// J1939 CAN Connection
// ------------------------------------------------------------------------
#include "CAN.h"

#include "ARD1939ESP32S3.h"

#include <inttypes.h>

// ------------------------------------------------------------------------
// CAN message ring buffer setup
// ------------------------------------------------------------------------
#define CANMSGBUFFERSIZE 10
struct CANMsg {
  long lID;
  unsigned char pData[8];
  int nDataLen;
};
CANMsg CANMsgBuffer[CANMSGBUFFERSIZE];
int nWritePointer;
int nReadPointer;

// ------------------------------------------------------------------------
// Initialize the CAN controller
// ------------------------------------------------------------------------
byte canInit(int baudRate) {
  // Default settings
  nReadPointer = 0;
  nWritePointer = 0;

  // Initialize the CAN controller
  return CAN.begin(baudRate) ? 0 : 1; // 0 = success, 1 = failure

} // end canInitialize

// ------------------------------------------------------------------------
// Check CAN controller for error
// ------------------------------------------------------------------------
byte canCheckError(void) {
  return 0; // Assume no error for now
} // end canCheckError

// ------------------------------------------------------------------------
// Transmit CAN message
// ------------------------------------------------------------------------
byte canTransmit(long lID, unsigned char *pData, int nDataLen) {
  CANMessage message;
  message.id = lID;
  message.length = nDataLen;
  memcpy(message.data, pData, nDataLen);

  return CAN.send(message) ? 0 : 1; // 0 = success, 1 = failure

} // end canTransmit

// ------------------------------------------------------------------------
// Receive CAN message
// ------------------------------------------------------------------------
byte canReceive(long *lID, unsigned char *pData, int *nDataLen) {
  // if (!CAN.available()) {
  //       return 1; // No message
  //   }
  CANMessage message = CAN.receive();
  // check if message id is a valid j1939 id
  if (message.id < 0x00F00000 || message.id > 0x1FFFFFFF) {
    return 1; // Ignore invalid message
  }

  Serial.printf("Received message with hex ID: %lx\n", message.id);
  Serial.printf("message length: %d\n", message.length);
  *lID = message.id;
  *nDataLen = message.length;
  memcpy(pData, message.data, *nDataLen);

  return 0; // Success

} // end canReceive

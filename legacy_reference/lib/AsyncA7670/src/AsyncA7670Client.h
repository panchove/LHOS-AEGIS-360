#ifndef ASYNC_A7670_CLIENT_H
#define ASYNC_A7670_CLIENT_H

#include "AsyncA7670.h"
#include "AsyncA7670Fifo.h"

#include <Arduino.h>
#include <Client.h>

class AsyncA7670Client : public Client {
  friend class AsyncA7670;
  typedef AsyncA7670Fifo<uint8_t, ASYNC_A7670_RX_BUFFER> RxFifo;

public:
  explicit AsyncA7670Client(AsyncA7670 &modem, uint8_t mux = 0);
  ~AsyncA7670Client();

  // Client interface implementation
  int connect(IPAddress ip, uint16_t port) override;
  int connect(const char *host, uint16_t port) override;
  size_t write(uint8_t) override;
  size_t write(const uint8_t *buf, size_t size) override;
  int available() override;
  int read() override;
  int read(uint8_t *buf, size_t size) override;
  int peek() override;
  void flush() override;
  void stop() override;
  uint8_t connected() override;
  operator bool() override;

  // Additional methods
  bool connect(const char *host, uint16_t port, int timeout_s);
  void setTimeout(uint32_t timeout);

private:
  AsyncA7670 &_modem;
  uint8_t _mux;
  uint32_t _timeout;
  bool _connected;

  // FIFO buffer for incoming data
  RxFifo _rxFifo;

  // Maintain TCP socket tracking
  uint32_t _lastMaintainTime;
};

#endif // ASYNC_A7670_CLIENT_H
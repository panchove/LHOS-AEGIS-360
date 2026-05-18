#include "AsyncA7670Client.h"

AsyncA7670Client::AsyncA7670Client(AsyncA7670 &modem, uint8_t mux)
    : _modem(modem), _mux(mux), _timeout(10000), _connected(false),
      _lastMaintainTime(0) {
  _rxFifo.clear();
  if (_mux < 10) {
    _modem.clients[_mux] = this;
  }
}

AsyncA7670Client::~AsyncA7670Client() {
  if (_mux < 10 && _modem.clients[_mux] == this) {
    _modem.clients[_mux] = nullptr;
  }
  stop();
  _rxFifo.clear();
}

int AsyncA7670Client::connect(IPAddress ip, uint16_t port) {
  return connect(ip.toString().c_str(), port);
}

int AsyncA7670Client::connect(const char *host, uint16_t port) {
  return connect(host, port, _timeout / 1000);
}

bool AsyncA7670Client::connect(const char *host, uint16_t port, int timeout_s) {
  if (_connected)
    stop();
  if (!_modem.modemConnect(host, port, _mux, false, timeout_s)) {
    return false;
  }

  uint32_t startTime = millis();
  uint32_t timeoutMs = timeout_s * 1000;

  while (millis() - startTime < timeoutMs) {
    if (_modem.modemGetConnected(_mux)) {
      _connected = true;
      return true;
    }
    TCPStatus status = _modem.tcpStatus[_mux];
    // Avoid false negatives: tcpStatus can transiently be set to CLOSED by
    // periodic CIPCLOSE? polling while a connect is still in flight.
    if (status == TCPStatus::ERROR) {
      _connected = false;
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  _connected = false;
  _modem.tcpStatus[_mux] = TCPStatus::CLOSED;
  return false;
}

size_t AsyncA7670Client::write(uint8_t byte) { return write(&byte, 1); }

size_t AsyncA7670Client::write(const uint8_t *buf, size_t size) {
  bool isConnected = _modem.modemGetConnected(_mux);
  if (!isConnected) {
    return 0;
  }
  _modem.maintainClient(_mux);
  return _modem.modemSend(buf, size, _mux);
}

int AsyncA7670Client::available() {
  _connected = _modem.modemGetConnected(_mux);
  if (!_connected)
    return 0;
  if (!_rxFifo.size()) {
    if (millis() - _lastMaintainTime > 500) {
      // setting got_data to true will tell maintain to run
      _modem.tcpConnections[_mux].got_data = true;
      _lastMaintainTime = millis();
    }
    _modem.maintainClient(_mux);
  }
  return static_cast<uint16_t>(_rxFifo.size()) +
         _modem.tcpConnections[_mux].modemAvailable;
}

int AsyncA7670Client::read() {
  if (!_connected)
    return -1;
  uint8_t c;
  if (read(&c, 1) == 1) {
    return c;
  }
  return -1;
}

int AsyncA7670Client::read(uint8_t *buf, size_t size) {
  if (!_connected || size == 0)
    return 0;
  size_t cnt = 0;
  _modem.maintainClient(_mux);
  while (cnt < size) {
    size_t chunk = min(size - cnt, _rxFifo.size());
    if (chunk > 0) {
      _rxFifo.get(buf, chunk);
      buf += chunk;
      cnt += chunk;
      continue;
    }
    if (millis() - _lastMaintainTime > 500) {
      // setting got_data to true will tell maintain to run
      _modem.tcpConnections[_mux].got_data = true;
      _lastMaintainTime = millis();
    }
    _modem.maintainClient(_mux);
    size_t sock_available = _modem.tcpConnections[_mux].modemAvailable;
    if (sock_available > 0) {
      int n =
        _modem.modemRead(min((size_t)_rxFifo.free(), sock_available), _mux);
      if (n == 0)
        break;
    } else {
      break;
    }
  }
  return cnt;
}

int AsyncA7670Client::peek() {
  if (!_connected)
    return -1;
  return _rxFifo.peek();
}

void AsyncA7670Client::flush() {
  // No action required
}

void AsyncA7670Client::stop() {
  _modem.modemDisconnect(_mux);
  _rxFifo.clear();
  _connected = false;
}

uint8_t AsyncA7670Client::connected() {
  _connected = _modem.modemGetConnected(_mux);
  return _connected ? 1 : 0;
}

AsyncA7670Client::operator bool() { return connected(); }

void AsyncA7670Client::setTimeout(uint32_t timeout) { _timeout = timeout; }

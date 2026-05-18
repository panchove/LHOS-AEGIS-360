#pragma once
#ifndef __ZIGBEELAYRZHUB_H__
#define __ZIGBEELAYRZHUB_H__

#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

// ------------ Simple ring buffer (single-producer / single-consumer)
// ------------
class ByteRing {
public:
  ByteRing(uint8_t *buf, size_t cap) : _buf(buf), _cap(cap) {}

  size_t capacity() const { return _cap; }
  size_t size() const { return _size; }
  size_t drops() const { return _drops; }

  // Push bytes; if overflow, drop oldest (advance tail)
  void push_drop_oldest(const uint8_t *data, size_t len) {
    if (!len)
      return;

    // if single chunk bigger than capacity: keep last cap bytes
    if (len > _cap) {
      data += (len - _cap);
      len = _cap;
    }

    // ensure space by dropping oldest
    while (_size + len > _cap) {
      // drop one byte at a time (simple). For performance you can drop in
      // blocks.
      _tail = (_tail + 1) % _cap;
      _size--;
      _drops++;
    }

    // write
    for (size_t i = 0; i < len; i++) {
      _buf[_head] = data[i];
      _head = (_head + 1) % _cap;
    }
    _size += len;
  }

  // Pop up to maxLen into out; returns bytes popped
  size_t pop(uint8_t *out, size_t maxLen) {
    size_t n = (_size < maxLen) ? _size : maxLen;
    for (size_t i = 0; i < n; i++) {
      out[i] = _buf[_tail];
      _tail = (_tail + 1) % _cap;
    }
    _size -= n;
    return n;
  }

private:
  uint8_t *_buf = nullptr;
  size_t _cap = 0;
  size_t _head = 0;
  size_t _tail = 0;
  size_t _size = 0;
  size_t _drops = 0;
};

// ------------ Globals / state ------------
static uint8_t *uplink_mem = nullptr;
static ByteRing *uplink_ring = nullptr;

static SemaphoreHandle_t ring_mutex;
static SemaphoreHandle_t net_mutex; // to serialize writes to TCP if needed

static volatile bool bridge_up = false;

static uint64_t uplink_rx_uart_bytes = 0;
static uint64_t uplink_tx_net_bytes = 0;
static uint64_t downlink_rx_net_bytes = 0;
static uint64_t downlink_tx_uart_bytes = 0;

void initZigbee();

#endif // __ZIGBEELAYRZHUB_H__

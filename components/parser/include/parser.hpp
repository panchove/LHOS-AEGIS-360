// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// ODIN Architecture Directive: C++17, no exceptions, no RTTI, heapless core
// Owner: ODIN | Last reviewed: 2026-05-18

#pragma once

#include <cstddef>
#include <cstdint>
#include "../pool/include/pool.hpp"

using parser_dispatch_cb_t = void(*)(pool_buf_t* buf);

class PacketParser {
public:
    PacketParser();
    ~PacketParser();

    bool init(parser_dispatch_cb_t cb, uint32_t timeout_ms = 500);
    void feed_byte(uint8_t b);
    void feed_bytes(const uint8_t* buf, size_t len);
    void reset();
    size_t get_crc_failures() const;
    size_t get_timeouts() const;

private:
    parser_dispatch_cb_t cb_;
    uint32_t timeout_ms_;
    size_t crc_failures_;
    size_t timeouts_;
    // simple internal buffer for assembly
    static constexpr size_t BUF_SZ = 1024;
    uint8_t buffer_[BUF_SZ];
    size_t buf_pos_;
};

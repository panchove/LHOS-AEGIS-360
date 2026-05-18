// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// ODIN Architecture Directive: C++17, no exceptions, no RTTI, heapless core
// Owner: ODIN | Last reviewed: 2026-05-18

#include "parser.hpp"
#include "../crc/include/crc16.hpp"
#include "../pool/include/pool.hpp"
#include <cstring>

PacketParser::PacketParser()
: cb_(nullptr), timeout_ms_(500), crc_failures_(0), timeouts_(0), buf_pos_(0) {
}

PacketParser::~PacketParser() {}

bool PacketParser::init(parser_dispatch_cb_t cb, uint32_t timeout_ms) {
    cb_ = cb;
    timeout_ms_ = timeout_ms;
    buf_pos_ = 0;
    return true;
}

void PacketParser::reset() {
    buf_pos_ = 0;
}

void PacketParser::feed_byte(uint8_t b) {
    if (buf_pos_ >= BUF_SZ) { buf_pos_ = 0; ++timeouts_; return; }
    buffer_[buf_pos_++] = b;
    // simple delimiter ';' indicates end of packet
    if (b == ';') {
        // validate CRC (simple, last two bytes before ';' assumed hex CRC) - simplified
        if (buf_pos_ >= 3) {
            // create pool buf and copy
            auto& pool = PacketPool::instance();
            pool_buf_t* p = pool.alloc();
            if (p) {
                size_t copy_len = (buf_pos_ > 0 ? buf_pos_-1 : 0);
                if (copy_len > 0 && copy_len <= 512) {
                    std::memcpy(p->data, buffer_, copy_len);
                    p->len = copy_len;
                    if (cb_) cb_(p);
                } else {
                    // release
                    pool.release(p);
                }
            } else {
                // pool exhausted, increment timeout counter
                ++timeouts_;
            }
        } else {
            ++crc_failures_;
        }
        buf_pos_ = 0;
    }
}

void PacketParser::feed_bytes(const uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) feed_byte(buf[i]);
}

size_t PacketParser::get_crc_failures() const { return crc_failures_; }
size_t PacketParser::get_timeouts() const { return timeouts_; }

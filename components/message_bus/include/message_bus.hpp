// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// ODIN Architecture Directive: C++17, no exceptions, no RTTI, heapless core
// Owner: ODIN | Last reviewed: 2026-05-18

#pragma once

#include "../pool/include/pool.hpp"

class MessageBus {
public:
    static MessageBus& instance();
    // Take ownership of buffer; returns true if accepted
    bool publish(pool_buf_t* buf);
private:
    MessageBus();
    ~MessageBus();
    MessageBus(const MessageBus&) = delete;
    MessageBus& operator=(const MessageBus&) = delete;
};

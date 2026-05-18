// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// ODIN Architecture Directive: C++17, no exceptions, no RTTI, heapless core
// Owner: ODIN | Last reviewed: 2026-05-18

#pragma once

#include <cstddef>
#include <cstdint>

struct pool_buf_t {
    uint8_t* data;
    size_t len;
    uint32_t magic; // debug/tracing
};

class PacketPool {
public:
    static PacketPool& instance();

    bool init(void* psram_buffer, size_t num_buffers, size_t buffer_size);
    pool_buf_t* alloc();
    void release(pool_buf_t* buf);
    size_t free_count() const;
    size_t total_count() const;

private:
    PacketPool();
    ~PacketPool();
    // non-copyable
    PacketPool(const PacketPool&) = delete;
    PacketPool& operator=(const PacketPool&) = delete;

    struct Slot;
    Slot* slots_;
    size_t total_;
    size_t buf_size_;
    // If PacketPool allocates PSRAM itself, track ownership to free at shutdown
    bool psram_owned_;
    void* psram_ptr_;
};

// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// ODIN Architecture Directive: C++17, no exceptions, no RTTI, heapless core
// Owner: ODIN | Last reviewed: 2026-05-18

#include "pool.hpp"
#include <atomic>
#include <cstring>
#include <esp_heap_caps.h>

struct PacketPool::Slot {
    alignas(8) uint8_t* data;
    std::atomic<bool> used;
    pool_buf_t meta;
};

PacketPool::PacketPool()
    : slots_(nullptr), total_(0), buf_size_(0), psram_owned_(false), psram_ptr_(nullptr) {}

PacketPool::~PacketPool() {
    if (slots_) heap_caps_free(slots_);
    if (psram_owned_ && psram_ptr_) heap_caps_free(psram_ptr_);
}

PacketPool& PacketPool::instance() {
    static PacketPool inst;
    return inst;
}

bool PacketPool::init(void* psram_buffer, size_t num_buffers, size_t buffer_size) {
    if (num_buffers == 0 || buffer_size == 0) return false;
    total_ = num_buffers;
    buf_size_ = buffer_size;
    // allocate slot metadata in internal RAM
    slots_ = reinterpret_cast<Slot*>(heap_caps_malloc(sizeof(Slot) * num_buffers, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!slots_) return false;

    uint8_t* base = nullptr;
    if (psram_buffer) {
        base = reinterpret_cast<uint8_t*>(psram_buffer);
        psram_owned_ = false;
        psram_ptr_ = nullptr;
    } else {
        // allocate the buffer region in PSRAM explicitly
        size_t total_size = num_buffers * buffer_size;
        base = reinterpret_cast<uint8_t*>(heap_caps_malloc(total_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!base) {
            // allocation failed
            heap_caps_free(slots_);
            slots_ = nullptr;
            return false;
        }
        psram_owned_ = true;
        psram_ptr_ = base;
    }

    for (size_t i = 0; i < num_buffers; ++i) {
        slots_[i].data = base + (i * buffer_size);
        slots_[i].used.store(false);
        slots_[i].meta.data = slots_[i].data;
        slots_[i].meta.len = 0;
        slots_[i].meta.magic = 0x50504F4F; // 'POOO'
    }
    return true;
}

pool_buf_t* PacketPool::alloc() {
    for (size_t i = 0; i < total_; ++i) {
        bool expected = false;
        if (slots_[i].used.compare_exchange_strong(expected, true)) {
            slots_[i].meta.len = 0;
            return &slots_[i].meta;
        }
    }
    return nullptr;
}

void PacketPool::release(pool_buf_t* buf) {
    if (!buf) return;
    // find slot by data pointer
    for (size_t i = 0; i < total_; ++i) {
        if (slots_[i].meta.data == buf->data) {
            slots_[i].meta.len = 0;
            slots_[i].used.store(false);
            return;
        }
    }
}

size_t PacketPool::free_count() const {
    size_t c = 0;
    for (size_t i = 0; i < total_; ++i) if (!slots_[i].used.load()) ++c;
    return c;
}

size_t PacketPool::total_count() const { return total_; }

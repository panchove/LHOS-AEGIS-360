#include "pool.hpp"
#include <cstdlib>
#include <new>

PacketPool& PacketPool::instance() {
    static PacketPool p;
    return p;
}

PacketPool::~PacketPool() {
    if (owned_ && slots_) {
        for (size_t i = 0; i < num_buffers_; ++i) {
            free(slots_[i].buffer);
        }
        free(slots_);
    }
}

bool PacketPool::init(void* external_buffer, size_t num_buffers, size_t buffer_size) {
    if (slots_) return false; // already inited
    num_buffers_ = num_buffers;
    buffer_size_ = buffer_size;
    slots_ = static_cast<Slot*>(calloc(num_buffers_, sizeof(Slot)));
    if (!slots_) return false;
    owned_ = true;
    for (size_t i = 0; i < num_buffers_; ++i) {
        slots_[i].buffer = static_cast<uint8_t*>(malloc(buffer_size_));
        slots_[i].used.store(false);
        slots_[i].magic = 0x0;
        if (!slots_[i].buffer) return false;
        free_count_.fetch_add(1);
    }
    return true;
}

pool_buf_t* PacketPool::alloc() {
    for (size_t i = 0; i < num_buffers_; ++i) {
        bool expected = false;
        if (slots_[i].used.compare_exchange_strong(expected, true)) {
            slots_[i].magic = 0xDEADBEEF;
            alloc_count_.fetch_add(1);
            free_count_.fetch_sub(1);
            pool_buf_t* b = static_cast<pool_buf_t*>(malloc(sizeof(pool_buf_t)));
            if (!b) return nullptr;
            b->data = slots_[i].buffer;
            b->len = 0;
            b->magic = slots_[i].magic;
            return b;
        }
    }
    return nullptr;
}

void PacketPool::release(pool_buf_t* buf) {
    if (!buf) return;
    // find slot by pointer
    for (size_t i = 0; i < num_buffers_; ++i) {
        if (slots_[i].buffer == buf->data) {
            if (!slots_[i].used.load()) {
                // double free
                return;
            }
            slots_[i].used.store(false);
            slots_[i].magic = 0x0;
            free_count_.fetch_add(1);
            alloc_count_.fetch_sub(1);
            free(buf);
            return;
        }
    }
    // not found: free struct
    free(buf);
}

size_t PacketPool::free_count() const { return free_count_.load(); }
size_t PacketPool::total_count() const { return num_buffers_; }
size_t PacketPool::alloc_count() const { return alloc_count_.load(); }
size_t PacketPool::leak_count() const { return alloc_count_.load(); }
bool PacketPool::is_valid(const pool_buf_t* buf) const {
    if (!buf) return false;
    for (size_t i = 0; i < num_buffers_; ++i) {
        if (slots_[i].buffer == buf->data) return slots_[i].magic == 0xDEADBEEF;
    }
    return false;
}

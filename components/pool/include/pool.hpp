// Minimal PacketPool API
#pragma once
#include <cstddef>
#include <cstdint>

namespace aegis {

struct pool_buf_t {
    uint8_t* data;
    size_t len;
};

class PacketPool {
public:
    PacketPool() = default;
    ~PacketPool();

    // backing is optional; if nullptr, pool will allocate internally (PSRAM when available)
    bool init(void* backing, size_t count, size_t buf_size);
    pool_buf_t* alloc();
    void release(pool_buf_t* b);
    size_t free_count() const;
    size_t total_count() const;

private:
    struct Slot {
        uint8_t* data = nullptr;
        bool used = false;
        size_t size = 0;
        // view returned to callers (stable per-slot)
        pool_buf_t view;
    };

    Slot* slots_ = nullptr;
    // If pool allocated a contiguous backing region, keep pointer here
    void* raw_memory_ = nullptr;
    // Who owns raw_memory_ (true if pool allocated it)
    bool owns_raw_memory_ = false;
    // Whether buffers are placed in PSRAM
    bool use_psram_ = false;
    size_t count_ = 0;
    size_t buf_size_ = 0;
};

} // namespace aegis

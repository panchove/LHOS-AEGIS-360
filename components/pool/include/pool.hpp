#pragma once
#include <cstdint>
#include <cstddef>
#include <atomic>

struct pool_buf_t {
    uint8_t* data;
    size_t len;
    uint32_t magic;  // 0xDEADBEEF if valid
};

class PacketPool {
public:
    static PacketPool& instance();
    bool init(void* external_buffer = nullptr,
              size_t num_buffers = 16,
              size_t buffer_size = 512);
    pool_buf_t* alloc();
    void release(pool_buf_t* buf);
    size_t free_count() const;
    size_t total_count() const;
    size_t alloc_count() const;
    size_t leak_count() const;
    bool is_valid(const pool_buf_t* buf) const;

private:
    PacketPool() = default;
    ~PacketPool();
    PacketPool(const PacketPool&) = delete;
    PacketPool& operator=(const PacketPool&) = delete;

    struct Slot {
        uint8_t* buffer;
        std::atomic<bool> used;
        uint32_t magic;
    };

    Slot* slots_{nullptr};
    size_t num_buffers_{0};
    size_t buffer_size_{0};
    std::atomic<size_t> alloc_count_{0};
    std::atomic<size_t> free_count_{0};
    bool use_psram_{false};
    bool owned_{false};
    void* psram_ptr_{nullptr};
};

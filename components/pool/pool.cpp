#include "pool.hpp"
#include <cstdlib>
#include <new>
#include <atomic>

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
// Prefer internal RAM allocation when PSRAM is not available. Using SPIRAM-only
// flags may fail on devices without PSRAM (returns NULL). Use INTERNAL|8BIT.
#define AEGIS_MALLOC(sz) heap_caps_malloc((sz), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
#define AEGIS_FREE(ptr) heap_caps_free(ptr)
#else
#define AEGIS_MALLOC(sz) malloc(sz)
#define AEGIS_FREE(ptr) free(ptr)
#endif

using namespace aegis;

PacketPool::~PacketPool()
{
    if (slots_) {
        // If we allocated a contiguous raw buffer, free it once.
        if (raw_memory_ && owns_raw_memory_) {
            heap_caps_free(raw_memory_);
            raw_memory_ = nullptr;
            owns_raw_memory_ = false;
        } else {
            // Otherwise free per-slot allocations (rare path)
            for (size_t i = 0; i < count_; ++i) {
                if (slots_[i].data) heap_caps_free(slots_[i].data);
            }
        }
        heap_caps_free(slots_);
        slots_ = nullptr;
    }
}

bool PacketPool::init(void* backing, size_t count, size_t buf_size)
{
    if (count == 0 || buf_size == 0) return false;
    count_ = count;
    buf_size_ = buf_size;

    // allocate slot array in internal RAM (small)
    slots_ = static_cast<Slot*>(heap_caps_malloc(sizeof(Slot) * count_, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!slots_) return false;

    // If caller provided backing memory, use it (do not take ownership)
    if (backing) {
        raw_memory_ = backing;
        owns_raw_memory_ = false;
        use_psram_ = false; // caller decides
    } else {
        // Try PSRAM first
        size_t total = count_ * buf_size_;
        void* raw = heap_caps_malloc(total, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (raw) {
            raw_memory_ = raw;
            owns_raw_memory_ = true;
            use_psram_ = true;
            ESP_LOGI("PacketPool", "Using PSRAM for buffers: %u x %u = %zu bytes", (unsigned)count_, (unsigned)buf_size_, total);
        } else {
            // Fallback to internal DRAM
            raw = heap_caps_malloc(total, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            if (!raw) {
                heap_caps_free(slots_);
                slots_ = nullptr;
                return false;
            }
            raw_memory_ = raw;
            owns_raw_memory_ = true;
            use_psram_ = false;
            ESP_LOGW("PacketPool", "PSRAM not available, using DRAM for buffers");
        }
    }

    // Initialize per-slot metadata and point into backing buffer
    for (size_t i = 0; i < count_; ++i) {
        slots_[i].data = static_cast<uint8_t*>(raw_memory_) + (i * buf_size_);
        slots_[i].size = buf_size_;
        slots_[i].used = false;
        slots_[i].view.data = slots_[i].data;
        slots_[i].view.len = slots_[i].size;
    }
    return true;
}

pool_buf_t* PacketPool::alloc()
{
    for (size_t i = 0; i < count_; ++i) {
        bool expected = false;
        if (!slots_[i].used) {
            if (__atomic_compare_exchange_n(&slots_[i].used, &expected, true, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                // return the stable per-slot view
                return &slots_[i].view;
            }
        }
    }
    return nullptr;
}

void PacketPool::release(pool_buf_t* b)
{
    if (!b) return;
    uint8_t* ptr = b->data;
    for (size_t i = 0; i < count_; ++i) {
        if (slots_[i].data == ptr) {
            __atomic_store_n(&slots_[i].used, false, __ATOMIC_SEQ_CST);
            return;
        }
    }
}

size_t PacketPool::free_count() const
{
    size_t freec = 0;
    for (size_t i = 0; i < count_; ++i) if (!slots_[i].used) ++freec;
    return freec;
}

size_t PacketPool::total_count() const { return count_; }

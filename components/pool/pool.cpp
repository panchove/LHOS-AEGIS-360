#include "pool.hpp"
#include <cstdlib>
#include <new>
#include <atomic>

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#define AEGIS_MALLOC(sz) heap_caps_malloc((sz), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define AEGIS_FREE(ptr) heap_caps_free(ptr)
#else
#define AEGIS_MALLOC(sz) malloc(sz)
#define AEGIS_FREE(ptr) free(ptr)
#endif

using namespace aegis;

PacketPool::~PacketPool()
{
    if (slots_) {
        for (size_t i = 0; i < count_; ++i) {
            if (slots_[i].data) AEGIS_FREE(slots_[i].data);
        }
        AEGIS_FREE(slots_);
        slots_ = nullptr;
    }
}

bool PacketPool::init(void* backing, size_t count, size_t buf_size)
{
    if (count == 0 || buf_size == 0) return false;
    count_ = count;
    buf_size_ = buf_size;

    // allocate slot array in normal RAM (small)
    slots_ = static_cast<Slot*>(AEGIS_MALLOC(sizeof(Slot) * count_));
    if (!slots_) return false;
    // placement-new each slot
    for (size_t i = 0; i < count_; ++i) {
        slots_[i].data = static_cast<uint8_t*>(AEGIS_MALLOC(buf_size_));
        if (!slots_[i].data) {
            // cleanup
            for (size_t j = 0; j < i; ++j) AEGIS_FREE(slots_[j].data);
            AEGIS_FREE(slots_);
            slots_ = nullptr;
            return false;
        }
        slots_[i].size = buf_size_;
        slots_[i].used = false;
    }
    return true;
}

pool_buf_t* PacketPool::alloc()
{
    for (size_t i = 0; i < count_; ++i) {
        bool expected = false;
        // simple atomic check via __atomic builtins
        if (!slots_[i].used) {
            // try to claim
            if (__atomic_compare_exchange_n(&slots_[i].used, &expected, true, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                // return view
                pool_buf_t* b = reinterpret_cast<pool_buf_t*>(slots_[i].data); // temporary alias
                // create a small meta struct on stack
                static thread_local pool_buf_t meta;
                meta.data = slots_[i].data;
                meta.len = slots_[i].size;
                return &meta;
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

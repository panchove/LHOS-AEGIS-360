---
doc_id: SDP.3.POOL.001
doc_type: INTERFACE
version: 0.1.0
status: DRAFT
owner: ODIN
last_reviewed: 2026-05-19
---

# PacketPool API

## Scope
Pool de buffers heapless en PSRAM (fallback a DRAM).

## Requirements
- 16 buffers de 512 bytes
- Alloc/Release sin malloc en hot-paths
- Detección de doble liberación (debug)
- Estadísticas (free/alloc/leak count)

## Interface Specs
```cpp
class PacketPool {
public:
    static PacketPool& instance();
    bool init(void* external_buffer = nullptr, 
              size_t num_buffers = 16, 
              size_t buffer_size = 512);
    pool_buf_t* alloc();
    void release(pool_buf_t* buf);
    size_t free_count() const;
    size_t alloc_count() const;
    bool is_valid(const pool_buf_t* buf) const;
};
```

## Verification
- `alloc()` → buffer válido
- `free_count()` disminuye después de alloc
- Doble release → `is_valid()` false

## Traceability
- FR-003 (Packet Pool)
- NFR-001 (Heapless core)

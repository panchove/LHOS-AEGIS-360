#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

namespace aegis {

class PacketParser {
public:
    using Callback = std::function<void(const uint8_t*, size_t)>;
    PacketParser() = default;
    void init(Callback cb, uint32_t timeout_ms);
    void feed_byte(uint8_t b);
    void feed_bytes(const uint8_t* data, size_t len);
    void reset();

private:
    Callback cb_;
    enum State { Idle, InTag, InBody } state_ = Idle;
    static const size_t BUF_SZ = 2048;
    uint8_t buf_[BUF_SZ];
    size_t pos_ = 0;
    char tag_open_[4] = {0};
};

} // namespace aegis

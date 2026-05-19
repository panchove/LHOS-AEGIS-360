#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include "pool.hpp"

enum class ParserState : uint8_t {
    IDLE,
    HEADER,
    BODY,
    CRC,
    DISPATCH,
    ERROR
};

using parser_dispatch_cb_t = void(*)(pool_buf_t* buf);

class PacketParser {
public:
    PacketParser();
    ~PacketParser();
    bool init(parser_dispatch_cb_t cb, uint32_t timeout_ms = 500);
    void feed_byte(uint8_t b);
    void feed_bytes(const uint8_t* data, size_t len);
    void reset();
    size_t get_packets_ok() const;
    size_t get_crc_failures() const;
    size_t get_timeouts() const;
    void check_timeout();

private:
    void process_byte(uint8_t b);
    void dispatch_packet();
    bool validate_crc(const uint8_t* data, size_t len, uint16_t expected);

    ParserState state_;
    parser_dispatch_cb_t callback_;
    pool_buf_t* current_buf_;
    uint8_t header_[8];
    size_t header_len_;
    uint8_t crc_hex_[5];
    size_t crc_len_;
    uint32_t last_activity_;
    uint32_t timeout_ms_;
    size_t packets_ok_;
    size_t crc_failures_;
    size_t timeouts_;
};

#include "parser.hpp"
#include "crc16.hpp"
#include <cstring>
#include <cctype>
#include <ctime>

PacketParser::PacketParser()
    : state_(ParserState::IDLE), callback_(nullptr), current_buf_(nullptr), header_len_(0), crc_len_(0), last_activity_(0), timeout_ms_(500), packets_ok_(0), crc_failures_(0), timeouts_(0) {}

PacketParser::~PacketParser() { reset(); }

bool PacketParser::init(parser_dispatch_cb_t cb, uint32_t timeout_ms) {
    callback_ = cb;
    timeout_ms_ = timeout_ms;
    return true;
}

void PacketParser::feed_bytes(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) process_byte(data[i]);
}

void PacketParser::feed_byte(uint8_t b) { process_byte(b); }

void PacketParser::reset() {
    if (current_buf_) {
        PacketPool::instance().release(current_buf_);
        current_buf_ = nullptr;
    }
    state_ = ParserState::IDLE;
    header_len_ = 0;
    crc_len_ = 0;
}

void PacketParser::process_byte(uint8_t b) {
    last_activity_ = (uint32_t)time(nullptr);
    switch (state_) {
        case ParserState::IDLE:
            if (b == '<') {
                // start header
                header_len_ = 0;
                state_ = ParserState::HEADER;
            }
            break;
        case ParserState::HEADER:
            header_[header_len_++] = b;
            if (b == '>') {
                // allocate buffer for body
                current_buf_ = PacketPool::instance().alloc();
                if (!current_buf_) { state_ = ParserState::ERROR; return; }
                state_ = ParserState::BODY;
            } else if (header_len_ >= sizeof(header_)) {
                state_ = ParserState::ERROR;
            }
            break;
        case ParserState::BODY:
            if (b == '<') {
                // possible closing tag; read until '>' into header_
                header_len_ = 0;
                header_[header_len_++] = b;
                state_ = ParserState::HEADER;
            } else {
                // append to buffer
                if (current_buf_->len < 65535) {
                    current_buf_->data[current_buf_->len++] = b;
                }
            }
            break;
        default:
            break;
    }
}

bool PacketParser::validate_crc(const uint8_t* data, size_t len, uint16_t expected) {
    uint16_t c = crc16::ccitt_false(data, len);
    return c == expected;
}

void PacketParser::dispatch_packet() {
    if (callback_ && current_buf_) {
        callback_(current_buf_);
        current_buf_ = nullptr;
    }
}

size_t PacketParser::get_packets_ok() const { return packets_ok_; }
size_t PacketParser::get_crc_failures() const { return crc_failures_; }
size_t PacketParser::get_timeouts() const { return timeouts_; }

void PacketParser::check_timeout() {
    uint32_t now = (uint32_t)time(nullptr);
    if (state_ != ParserState::IDLE && now - last_activity_ > timeout_ms_/1000) {
        timeouts_++;
        reset();
    }
}

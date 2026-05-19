#include "parser.hpp"
#include <cstring>
#include <cstdio>
#include <string>

namespace aegis {

void PacketParser::init(Callback cb, uint32_t /*timeout_ms*/)
{
    cb_ = cb;
    reset();
}

void PacketParser::reset()
{
    state_ = Idle;
    pos_ = 0;
    memset(buf_, 0, BUF_SZ);
}

void PacketParser::feed_byte(uint8_t b)
{
    switch (state_) {
    case Idle:
        if (b == '<') {
            state_ = InTag;
            pos_ = 0;
        }
        break;
    case InTag:
        if (b == '>') {
            // tag closed, start body
            state_ = InBody;
            pos_ = 0;
        } else {
            if (pos_ < sizeof(tag_open_)-1) tag_open_[pos_++] = (char)b;
        }
        break;
    case InBody:
        if (b == '<') {
            // possible closing
            // peek next will be '/'
            state_ = InTag;
            // we reuse tag buffer to detect '/'
            pos_ = 0;
        } else {
            if (pos_ < BUF_SZ) buf_[pos_++] = b;
        }
        break;
    }
}

void PacketParser::feed_bytes(const uint8_t* data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        // very simplified: detect pattern <Px>...<\/Px>
        // append into temporary buffer and call cb_ when we detect closing sequence 
        static const char close_pat[] = "</Pb>"; // accept Pb as example
        // naive approach: if sequence "</" found and then tag equal, we emit
        // For brevity, implement a simple handler for example tags "Pb" "Px" etc.
        // We'll accumulate and if we see '<' followed by '/' we check for tag then '>'
        // Simpler: if input contains substring "</Pb>" emit current buffer
        // Append to local buffer
        // For this minimal implementation, build a temporary string and search
        static std::string acc;
        acc.push_back((char)data[i]);
        if (acc.size() >= 5) {
            if (acc.size() >= 5 && acc.substr(acc.size()-5) == std::string("</Pb>")) {
                // find start of body by locating ">" preceding content
                auto start = acc.find('>');
                if (start != std::string::npos) {
                    size_t body_start = start + 1;
                    size_t body_len = acc.size() - 5 - body_start;
                    if (cb_) cb_((const uint8_t*)acc.data()+body_start, body_len);
                }
                acc.clear();
            }
        }
    }
}

} // namespace aegis
